Loosely following [2D Game Physics Programming](https://pikuma.com/courses/game-physics-engine-programming)

# Build for Linux

```sh
$ ./build.sh
$ cd build
$ # If you do not have sdl3 installed on system, make sure linux can find built
$ # from source.
$ if ! pkg-config sdl3; then export LD_LIBRARY_PATH="$PWD/3rdparty/SDL3-3.2.8-install/lib64"; fi
$ ./game
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

# Build for Windows

Core applications come from [cosmopolitan libc](https://justine.lol/cosmopolitan/index.html).

You can download individual files from https://cosmo.zip/pub/cosmos/bin
or extract zip from https://cosmo.zip/pub/cosmos/zip/cosmos.zip

For building our game we need:
- bash
- cp
- cut
- date
- echo
- head
- mkdir
- mv

Compiler we support is clang. You can download prebuilt from [llvm/llvm-project](https://github.com/llvm/llvm-project/releases/latest).

You also need [Build Tools for Visual Studio 2022](https://aka.ms/vs/17/release/vs_BuildTools.exe).

```
winget install Microsoft.VisualStudio.2022.BuildTools --force --override "--wait --passive --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --add Microsoft.VisualStudio.Component.Windows10SDK.16299"
```

Within [Microsoft's docs](https://learn.microsoft.com/en-us/visualstudio/install/workload-component-id-vs-build-tools) you can find the IDs of the features available for the Visual Studio Build Tools and in [this section](https://learn.microsoft.com/en-us/visualstudio/install/use-command-line-parameters-to-install-visual-studio) also learn more about other parameters available for the installer.

```sh
$ export CC=clang.exe
$ ./build.sh
$ cd build
$ ./game
```
