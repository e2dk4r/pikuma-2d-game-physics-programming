Loosely following [2D Game Physics Programming](https://pikuma.com/courses/game-physics-engine-programming)

# Gotchas

1. On linux, setting `LD_LIBRARY_PATH=./` causes app to segfault while recompiling.
   
   App gets SIGSEGV.
   Reproduce Steps:
   ```
   $ ./build.sh
   $ cd build
   $ export 'LD_LIBRARY_PATH=./'
   $ ./game
   $ ./build.sh # recompile
   ```
