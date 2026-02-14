[[Back to main page](../README.md)]
# Building with host libraries

It is possible to build the emulator using libraries that are already installed on the system.

The libraries could for example be installed as DEB packages on a Linux system 
or as Brew packages on macOS (and more).

## Installing dependencies

The following dependencies are required.

 * `Boost`
   * `Boost.Log`
   * `Boost.Program_options`
   * `Boost.Circular_buffer`
 * `SDL2`
 * `SDL2_image`

That means that the following packages could be installed on a Debian-based Linux system.

```
$ apt install libboost-log-dev libboost-program-options-dev libboost-circular-buffer-dev
$ apt install libsdl2-dev libsdl2-image-dev
``` 

## Configuring with CMake

This should typically be done like the following.

```
$ mkdir build
$ cd build

$ cmake ..
```

If you want to run the cmake command with the option to use ninja instead of GNU Make,
then the following should work.

```
$ cmake -G Ninja ..
```

## Build

When the above is finished you can build the project with normal CMake build commands, as below.

```
$ make -j10
```

If you want to build with ninja (after configuring for Ninja use), then the following should work.

```
$ ninja
```

