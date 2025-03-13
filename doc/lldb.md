# lldb does not recognize our structures
`lldb` do not pretty print `v2` `v3` like structures.

If you want to pretty print used structures in this project, you can start `lldb` with our `lldbinit` file.

```sh
$ ./build.sh
$ cd build
$ # Start debug session
$ lldb -s ../.lldb/lldbinit -- ./game
```
