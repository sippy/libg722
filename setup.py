from sys import exit
from distutils.core import setup, Extension
from setuptools.command.test import test as TestCommand
from os.path import exists, realpath, dirname, join as path_join
from sys import argv as sys_argv
import numpy as np

mod_name = 'G722'
mod_name_dbg = mod_name + '_debug'

mod_dir = dirname(realpath(sys_argv[0]))
src_dir = './' if exists('g722_decode.c') else '../'
mod_fname = mod_name + '_mod.c'
mod_dir = '' if exists(mod_fname) else 'python/'

compile_args = [f'-I{src_dir}', '-flto', f'-I{np.get_include()}']
smap_fname = f'{mod_dir}symbols.map'
link_args = ['-flto', f'-Wl,--version-script={smap_fname}']
debug_cflags = ['-g3', '-O0', '-DDEBUG_MOD']
debug_link_args = ['-g3', '-O0']
mod_common_args = {
    'sources': [mod_dir + mod_fname, src_dir + 'g722_decode.c', src_dir + 'g722_encode.c'],
    'extra_compile_args': compile_args,
    'extra_link_args': link_args
}
mod_debug_args = mod_common_args.copy()
mod_debug_args['extra_compile_args'] = mod_debug_args['extra_compile_args'] + debug_cflags
mod_debug_args['extra_link_args'] = mod_debug_args['extra_link_args'] + debug_link_args

module1 = Extension(mod_name, **mod_common_args)
module2 = Extension(mod_name_dbg, **mod_debug_args)

requirements = [x.strip() for x in open(mod_dir + "requirements.txt", "r").readlines()]
with open(src_dir + "README.md", "r") as fh:
    long_description = fh.read()

kwargs = {
      'name':mod_name,
      'version':'1.1.0',
      'description':'This is a package for G.722 module',
      'long_description': long_description,
      'long_description_content_type': "text/markdown",
      'author':'Maksym Sobolyev',
      'author_email':'sobomax@sippysoft.com',
      'url':'https://github.com/sippy/libg722',
      'ext_modules': [module1, module2],
      'python_requires': '>=3.10',
      'install_requires': requirements,
      'classifiers': [
            'License :: Public Domain',
            'Operating System :: OS Independent',
            'Programming Language :: C',
            'Programming Language :: Python'
      ]
}

setup (**kwargs)
