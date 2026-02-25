#!/usr/bin/env python

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from os.path import exists, realpath, dirname
from os import environ
from subprocess import run, PIPE
from sys import argv as sys_argv, path as sys_path

from sysconfig import get_platform

sys_path.insert(0, realpath(dirname(__file__)))
from build_tools.CheckVersion import CheckVersion

NO_NUMPY_ENVVAR = "LIBG722_NO_NUMPY"
BUILD_MODE_ENVVAR = "LIBG722_BUILD_MODE"


def env_flag_is_true(name):
    value = environ.get(name)
    if value is None:
        return False
    return value.strip().lower() in {"1", "true", "yes", "on"}


def is_numpy_requirement(requirement):
    return requirement.split(";", 1)[0].strip().lower() == "numpy"


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

    mod_name = 'G722'
    mod_name_dbg = mod_name + '_debug'
    version = '1.2.3'
    repo_dir = realpath(dirname(__file__))
    with_numpy = not env_flag_is_true(NO_NUMPY_ENVVAR)
    build_mode = resolve_build_mode(repo_dir, version)
    is_debug_build = build_mode == "debug"

    mod_dir = dirname(realpath(sys_argv[0]))
    src_dir = './' if exists('g722_decode.c') else '../'
    mod_fname = mod_name + '_mod.c'
    mod_dir = '' if exists(mod_fname) else 'python/'

    class BuildExtWithOptionalNumpy(build_ext):
        def finalize_options(self):
            super().finalize_options()
            if not with_numpy:
                return
            try:
                import numpy as np
            except ImportError as exc:
                raise RuntimeError(
                    f"NumPy headers are required to build with NumPy support. "
                    f"Install numpy or set {NO_NUMPY_ENVVAR}=1 to build without NumPy support."
                ) from exc
            if self.include_dirs is None:
                self.include_dirs = []
            self.include_dirs.append(np.get_include())

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
        smap_fname = f'{mod_dir}symbols.map'
        link_args.append(f'-Wl,--version-script={smap_fname}')
    debug_cflags = ['-DDEBUG_MOD']
    debug_link_args = []
    if is_debug_build:
        debug_cflags = ['-g3', '-O0', '-DDEBUG_MOD']
        debug_link_args = ['-g3', '-O0']
    mod_common_args = {
        'sources': [mod_dir + mod_fname, src_dir + 'g722_decode.c', src_dir + 'g722_encode.c'],
        'include_dirs': [src_dir],
        'define_macros': [('WITH_NUMPY', '1' if with_numpy else '0')],
        'extra_compile_args': compile_args,
        'extra_link_args': link_args
    }
    mod_debug_args = mod_common_args.copy()
    mod_debug_args['extra_compile_args'] = mod_debug_args['extra_compile_args'] + debug_cflags
    mod_debug_args['extra_link_args'] = mod_debug_args['extra_link_args'] + debug_link_args

    module1 = Extension(mod_name, **mod_common_args)
    module2 = Extension(mod_name_dbg, **mod_debug_args)

    with open(mod_dir + "requirements.txt", "r") as req_fh:
        requirements = [x.strip() for x in req_fh.readlines() if x.strip()]
    if not with_numpy:
        requirements = [x for x in requirements if not is_numpy_requirement(x)]
    setup_requirements = [] if not with_numpy else ['numpy']
    with open(src_dir + "README.md", "r") as fh:
        long_description = fh.read()

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
        'setup_requires': setup_requirements,
        'install_requires': requirements,
        'cmdclass': {'checkversion': CheckVersion, 'build_ext': BuildExtWithOptionalNumpy},
        'license': 'Public-Domain',
        'classifiers': [
                'Operating System :: OS Independent',
                'Programming Language :: C',
                'Programming Language :: Python'
        ]
    }

    setup (**kwargs)

if __name__ == '__main__': main()
