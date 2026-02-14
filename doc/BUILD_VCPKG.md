[[Back to main page](../README.md)]
# Building with vcpkg

A good way to build Auric is to use **vcpkg**, which is a C/C++ package manager that 
simplifies dependency management and building on multiple platforms. 
You can find more information about vcpkg at: https://vcpkg.io/en/.

When using vcpkg the dependencies will be automatically downloaded and built, 
from a list of dependencies specified in the `vcpkg.json` file. 
This means that you don't have to worry about installing the dependencies manually,

## Prerequisites

You need a working vcpkg installation. 

The environment variable `VCPKG_ROOT` should point to the root of the vcpkg installation.

## Installing dependencies

The `vcpkg.json` file contains all the dependencies. To install them, do as follows.

```
$ vcpkg install
$ vcpkg integrate install
```

## Configuring with CMake

There is a predefined CMake preset for using vcpkg, use it as follows.

```
$ mkdir build
$ cd build
$ cmake --preset=vcpkg ..
```

## Build

When the above is finished you can build the project with normal CMake build commands, as below.

```
$ make -j10
```

