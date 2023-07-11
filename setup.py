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


with open('README.md', 'r', encoding='utf-8') as file:
    long_description = file.read()


setup(
    name='sysproxy',
    version='0.1.1',
    license='MIT',
    description='Python bindings for shadowsocks sysproxy utility.',
    long_description=long_description,
    long_description_content_type='text/markdown',
    author='Loren Eteval',
    author_email='loren.eteval@proton.me',
    url='https://github.com/LorenEteval/sysproxy',
    setup_requires=['pybind11'],
    install_requires=['pybind11'],
    ext_modules=[
        Extension(
            'sysproxy',
            ['sysproxy.cpp'],
            language='c++11',
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
        'License :: OSI Approved :: MIT License',
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
        'Operating System :: Microsoft :: Windows',
        'Topic :: Internet',
        'Topic :: Internet :: Proxy Servers',
    ],
    zip_safe=False,
)
