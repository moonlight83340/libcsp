from setuptools import setup, find_packages

setup(
    name='csp_moonlight',
    version='2.0.3',
    description='CSP Python bindings',
    long_description=open('README.md').read(),
    long_description_content_type='text/markdown',
    license='MIT',
    license_files=('LICENSE',),
    author='moonlight83340',
    author_email='gaetan.perrot@spacecubics.com',
    url='https://github.com/moonlight83340/libcsp',
    packages=find_packages(),  # Automatically finds packages in libcsp_py3
    data_files=[('csp_moonlight', ['csp_moonlight/libcsp_py3-ubuntu-24.04.so'])],  # Specifies the .so file
    include_package_data=True,
    classifiers=[
        'Programming Language :: Python :: 3',
        'License :: OSI Approved :: MIT License',
        'Operating System :: POSIX :: Linux',
    ],
    python_requires='>=3.5',
)
