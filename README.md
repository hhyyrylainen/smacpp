Static Memory Analyzer For C++
==============================

This tool is implemented as a clang plugin. Once the plugin is
integrated in your clang installation it will automatically work on
any code you compile with clang.

Dependencies
------------

- Tool for C++ development: compiler, git, make, cmake
- Clang with development headers installed and clang-tidy.
- Ruby for utility scripts
- Python3 for some scripts
- general-purpose-preprocessor
- Only tested in a Linux environment

Test Suites
-----------

These are automatically downloaded by the setup script or included as sub modules.

- https://github.com/JMoerman/JM2018TS
- https://samate.nist.gov/SRD/testsuites/juliet/Juliet_Test_Suite_v1.3_for_C_Cpp.zip


Building
--------
After cloning:

```sh
cd smacpp
git submodule update --init
./PreProcessData.rb
mkdir build
cd build
cmake ..
make
make test
sudo make install
```
