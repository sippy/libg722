#!/usr/bin/env python

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from os.path import exists, realpath, dirname, join as path_join
from os import environ
from subprocess import run, PIPE
from sys import path as sys_path

from sysconfig import get_platform

sys_path.insert(0, realpath(dirname(__file__)))
from build_tools.CheckVersion import CheckVersion

BUILD_MODE_ENVVAR = "LIBG722_BUILD_MODE"
PACKAGE_VARIANT_ENVVAR = "LIBG722_PACKAGE_VARIANT"


def infer_package_variant_from_metadata(repo_dir):
    fname = path_join(repo_dir, "PKG-INFO")
    if not exists(fname):
        return None
    with open(fname, "r", encoding="utf-8", errors="replace") as fh:
        for line in fh:
            if not line.startswith("Name:"):
                continue
            name = line.split(":", 1)[1].strip().lower().replace("_", "-")
            if name == "g722-numpy":
                return "numpy-addon"
            if name == "g722":
                return "core"
            break
    return None


def get_package_variant(repo_dir):
    value = environ.get(PACKAGE_VARIANT_ENVVAR)
    if value is None:
        inferred = infer_package_variant_from_metadata(repo_dir)
        if inferred is not None:
            return inferred
        return "core"
    value = value.strip().lower()
    aliases = {
        "main": "core",
        "base": "core",
        "addon": "numpy-addon",
        "numpy": "numpy-addon",
        "numpy_addon": "numpy-addon",
    }
    value = aliases.get(value, value)
    if value not in {"core", "numpy-addon"}:
        raise RuntimeError(
            f"Invalid {PACKAGE_VARIANT_ENVVAR}={value!r}. "
            f"Expected one of: core, numpy-addon."
        )
    return value


def get_build_mode_setting():
    value = environ.get(BUILD_MODE_ENVVAR, "auto").strip().lower()
    aliases = {
        "prod": "production",
        "release": "production",
        "dev": "debug",
    }
    value = aliases.get(value, value)
    if value not in {"auto", "debug", "production"}:
        raise RuntimeError(
            f"Invalid {BUILD_MODE_ENVVAR}={value!r}. "
            f"Expected one of: auto, debug, production."
        )
    return value


def git_has_diff_against_tag(repo_dir, tag):
    is_repo = run(
        ["git", "-C", repo_dir, "rev-parse", "--is-inside-work-tree"],
        stdout=PIPE,
        stderr=PIPE,
        text=True,
        check=False,
    )
    if is_repo.returncode != 0 or is_repo.stdout.strip() != "true":
        return None

    has_tag = run(
        ["git", "-C", repo_dir, "rev-parse", "-q", "--verify", f"refs/tags/{tag}"],
        stdout=PIPE,
        stderr=PIPE,
        text=True,
        check=False,
    )
    if has_tag.returncode != 0:
        return True

    diff = run(
        ["git", "-C", repo_dir, "diff", "--quiet", tag, "--", "."],
        stdout=PIPE,
        stderr=PIPE,
        text=True,
        check=False,
    )
    return diff.returncode != 0


def resolve_build_mode(repo_dir, version):
    setting = get_build_mode_setting()
    if setting != "auto":
        return setting
    has_diff = git_has_diff_against_tag(repo_dir, f"v{version}")
    if has_diff is None:
        return "production"
    return "debug" if has_diff else "production"


def main():

    mod_name = "G722"
    mod_name_dbg = mod_name + "_debug"
    version = '1.2.4'
    repo_dir = realpath(dirname(__file__))
    package_variant = get_package_variant(repo_dir)
    src_dir = "."
    py_src_dir = "python"

    mod_fname = mod_name + "_mod.c"

    readme_path = path_join(repo_dir, "README.md")
    if exists(readme_path):
        with open(readme_path, "r", encoding="utf-8", errors="replace") as fh:
            long_description = fh.read()
    else:
        long_description = "This is a package for G.722 module"

    if package_variant == "numpy-addon":
        class BuildExtWithNumpy(build_ext):
            def finalize_options(self):
                super().finalize_options()
                try:
                    import numpy as np
                except ImportError:
                    # During build requirement discovery NumPy may not be installed yet.
                    # setup_requires requests it; finalize_options runs again for build.
                    return
                if self.include_dirs is None:
                    self.include_dirs = []
                self.include_dirs.append(np.get_include())

        addon_module = Extension(
            "G722_numpy",
            sources=[path_join(py_src_dir, "G722_numpy_mod.c")],
            include_dirs=[py_src_dir],
        )
        kwargs = {
            "name": "G722-numpy",
            "version": version,
            "description": "Optional NumPy backend for G.722 module",
            "long_description": long_description,
            "long_description_content_type": "text/markdown",
            "author": "Maksym Sobolyev",
            "author_email": "sobomax@sippysoft.com",
            "url": "https://github.com/sippy/libg722",
            "ext_modules": [addon_module],
            "setup_requires": ["numpy"],
            "install_requires": [f"G722=={version}", "numpy"],
            "cmdclass": {"checkversion": CheckVersion, "build_ext": BuildExtWithNumpy},
            "license": "Public-Domain",
            "classifiers": [
                "Operating System :: OS Independent",
                "Programming Language :: C",
                "Programming Language :: Python",
            ],
        }
        setup(**kwargs)
        return

    build_mode = resolve_build_mode(repo_dir, version)
    is_debug_build = build_mode == "debug"

    is_win = get_platform().startswith('win')
    is_mac = get_platform().startswith('macosx-')

    compile_args = []
    if not is_win:
        compile_args.append('-flto')
    link_args = ['-flto',] if not is_win else []
    if is_debug_build:
        compile_args.extend(['-g3', '-O0'])
        link_args.extend(['-g3', '-O0'])
    else:
        compile_args.append('/O2' if is_win else '-O2')
        if not is_win:
            link_args.append('-O2')
    if not is_mac and not is_win:
        smap_fname = path_join(py_src_dir, "symbols.map")
        link_args.append(f'-Wl,--version-script={smap_fname}')
    debug_cflags = ['-DDEBUG_MOD']
    debug_link_args = []
    if is_debug_build:
        debug_cflags = ['-g3', '-O0', '-DDEBUG_MOD']
        debug_link_args = ['-g3', '-O0']
    mod_common_args = {
        'sources': [
            path_join(py_src_dir, mod_fname),
            path_join(src_dir, 'g722_decode.c'),
            path_join(src_dir, 'g722_encode.c'),
        ],
        'include_dirs': [src_dir, py_src_dir],
        'extra_compile_args': compile_args,
        'extra_link_args': link_args
    }
    mod_debug_args = mod_common_args.copy()
    mod_debug_args['extra_compile_args'] = mod_debug_args['extra_compile_args'] + debug_cflags
    mod_debug_args['extra_link_args'] = mod_debug_args['extra_link_args'] + debug_link_args

    module1 = Extension(mod_name, **mod_common_args)
    module2 = Extension(mod_name_dbg, **mod_debug_args)

    kwargs = {
        'name':mod_name,
        'version': version,
        'description':'This is a package for G.722 module',
        'long_description': long_description,
        'long_description_content_type': "text/markdown",
        'author':'Maksym Sobolyev',
        'author_email':'sobomax@sippysoft.com',
        'url':'https://github.com/sippy/libg722',
        'ext_modules': [module1, module2],
        'install_requires': [],
        'extras_require': {'numpy': [f'G722-numpy=={version}']},
        'cmdclass': {'checkversion': CheckVersion},
        'license': 'Public-Domain',
        'classifiers': [
                'Operating System :: OS Independent',
                'Programming Language :: C',
                'Programming Language :: Python'
        ]
    }

    setup (**kwargs)

if __name__ == '__main__': main()
