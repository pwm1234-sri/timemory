#!/usr/bin/env python

import os
import re
import sys
import sysconfig
import platform
import subprocess

from distutils.version import LooseVersion
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


# ------------------------------------------------------------------------------------- #
class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


# ------------------------------------------------------------------------------------- #
class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError(
                "CMake must be installed to build the following extensions: " +
                ", ".join(e.name for e in self.extensions))

        if platform.system() == "Windows":
            cmake_version = LooseVersion(re.search(r'version\s*([\d.]+)',
                                         out.decode()).group(1))
            if cmake_version < '3.1.3':
                raise RuntimeError("CMake >= 3.1.3 is required on Windows")

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(
            os.path.dirname(self.get_ext_fullpath(ext.name)))
        cmake_args = [#'-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
                      '-DPYTHON_EXECUTABLE=' + sys.executable,
                      #'-DPROJECT_OUTPUT_DIR=' + extdir,
                      '-DSETUP_PY=ON',
                      '-DCMAKE_INSTALL_PREFIX=' + extdir]

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        if platform.system() == "Windows":
            cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(
                cfg.upper(),
                extdir)]
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j4']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(
            env.get('CXXFLAGS', ''),
            self.distribution.get_version())
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args,
                              cwd=self.build_temp, env=env)
        subprocess.check_call(['cmake', '--build', '.'] + build_args,
                              cwd=self.build_temp)
        subprocess.check_call(['cmake', '--build', '.', '--target', 'install'],
                              cwd=self.build_temp)
        print()  # Add an empty line for cleaner output


# ------------------------------------------------------------------------------------- #
#setup(
    #name='python_cpp_example',
    #version='0.1',
    #author='Benjamin Jack',
    #author_email='benjamin.r.jack@gmail.com',
    #description='A hybrid Python/C++ test project',
    #long_description='',
    ## add extension module
    #ext_modules=[CMakeExtension('python_cpp_example')],
    ## add custom build_ext command
    #cmdclass=dict(build_ext=CMakeBuild),
    #zip_safe=False,
#)

# ------------------------------------------------------------------------------------- #
# calls the setup and declare our 'my_cool_package'
setup(name='timemory',
    version='0.1.dev0',
    author='Jonathan R. Madsen',
    author_email='jonrobm.programming@gmail.com',
    maintainer='Jonathan R. Madsen',
    maintainer_email='jonrobm.programming@gmail.com',
    contact='Jonathan R. Madsen',
    contact_email='jonrobm.programming@gmail.com',
    description='Python timing + memory manager',
    long_description=open('README.rst').read(),
    url='https://github.com/jrmadsen/TiMemory.git',
    license='MIT',
    # add extension module
    ext_modules=[CMakeExtension('TiMemory')],
    # add custom build_ext command
    cmdclass=dict(build_ext=CMakeBuild),
    zip_safe=False,
    # extra
    #install_requires=[ 'numpy', 'matplotlib', 'argparse', 'unittest' ],
    provides=[ 'timemory' ],
    keywords=[ 'timing', 'memory', 'auto-timers', 'signal', 'c++', 'cxx' ],
    python_requires='>=2.6',
)
