#!/usr/bin/env python

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from os.path import exists, realpath, dirname
from os import environ
from sys import argv as sys_argv, path as sys_path

from sysconfig import get_platform

sys_path.insert(0, realpath(dirname(__file__)))
from build_tools.CheckVersion import CheckVersion

NO_NUMPY_ENVVAR = "LIBG722_NO_NUMPY"


def env_flag_is_true(name):
    value = environ.get(name)
    if value is None:
        return False
    return value.strip().lower() in {"1", "true", "yes", "on"}


def is_numpy_requirement(requirement):
    return requirement.split(";", 1)[0].strip().lower() == "numpy"


def main():

    mod_name = 'G722'
    mod_name_dbg = mod_name + '_debug'
    with_numpy = not env_flag_is_true(NO_NUMPY_ENVVAR)

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
    if not is_mac and not is_win:
        smap_fname = f'{mod_dir}symbols.map'
        link_args.append(f'-Wl,--version-script={smap_fname}')
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
        'version':'1.2.2',
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
