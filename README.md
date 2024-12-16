Loosely following [2D Game Physics Programming](https://pikuma.com/courses/game-physics-engine-programming)

# Build for Linux

```sh
./build.sh
cd build
export LD_LIBRARY_PATH="$PWD/3rdparty/SDL3-3.1.6-install/lib64"
./game
```

## Gotchas

1. Setting `LD_LIBRARY_PATH=./` causes app to segfault while recompiling.

This was discovered while copying libSDL3.so to current directory, then
trying to hot reload game library.
   
Doing so will cause the app get `SIGSEGV` signal.

Reproduce Steps:

```sh
$ ./build.sh
$ cd build
$ export 'LD_LIBRARY_PATH=./'
$ ./game
$ ./build.sh # recompile
```
