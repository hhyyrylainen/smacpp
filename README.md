Static Memory Analyzer For C++
==============================

This tool is implemented as a clang plugin. Once the plugin is
integrated in your clang installation it will automatically work on
any code you compile with clang.


Building
--------
After cloning:

```sh
cd smacpp
mkdir build
cd build
cmake ..
make
make test
sudo make install
```
