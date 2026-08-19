from pathlib import Path

from setuptools import setup
from setuptools.extension import Extension


class Pybind11Include:
    """
    Helper class to determine the pybind11 include path

    The purpose of this class is to postpone importing pybind11
    until it is actually installed, so that the get_include()
    method can be invoked.
    """

    def __str__(self):
        import pybind11

        return pybind11.get_include()


long_description = (Path(__file__).parent / 'README.md').read_text(encoding='utf-8')


setup(
    name='sysproxy',
    version='0.2.0',
    license='MIT',
    description='Python bindings for shadowsocks sysproxy utility.',
    long_description=long_description,
    long_description_content_type='text/markdown',
    author='Loren Eteval',
    author_email='loren.eteval@proton.me',
    url='https://github.com/LorenEteval/sysproxy',
    python_requires='>=3.6',
    ext_modules=[
        Extension(
            'sysproxy',
            ['sysproxy.cpp'],
            language='c++',
            include_dirs=[
                # Path to pybind11 headers
                Pybind11Include(),
            ],
            define_macros=[('UNICODE', None), ('_UNICODE', None)],
            libraries=['Wininet', 'rasapi32', 'user32'],
        )
    ],
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'Intended Audience :: Developers',
        'Programming Language :: C++',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3 :: Only',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Programming Language :: Python :: 3.12',
        'Programming Language :: Python :: 3.13',
        'Programming Language :: Python :: 3.14',
        'Operating System :: Microsoft :: Windows',
        'Topic :: Internet',
        'Topic :: Internet :: Proxy Servers',
    ],
    zip_safe=False,
)
