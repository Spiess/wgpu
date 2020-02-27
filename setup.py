from setuptools import setup

setup(
    name='wgpu',
    version='0.1.2',
    description='A command line utility to show who is using which GPU and what they are doing - similar to w.',
    url='https://github.com/spiess/wgpu',
    author='Florian Spiess',
    author_email='f.spiess@unibas.ch',
    license='MIT License',
    packages=['wgpu'],
    install_requires=['nvidia-ml-py3'],
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Science/Research',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 3',
    ],
    entry_points={
        'console_scripts': [
            'wgpu = wgpu.__main__:run'
        ]
    }
)
