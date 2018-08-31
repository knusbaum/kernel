# KERNEL

This is an x86 kernel with an integrated lisp VM.
The lisp syntax is my own, but should not be too challenging.

Read the makefile to understand how it's built.

#### Basic build
You need to be on some sort of *nix to build

This will ask you for sudo permissions, because it mounts a fat32 volume.
If you prefer, you can read the `populate_disk` rule and execute the commands yourself.
```
$ make populate_disk
```

Then:
```
$ make run
```
This requires qemu to be installed on the system.
