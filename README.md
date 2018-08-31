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


## Notes and disclaimers

About reading files...

As of now, the `load-file` function doesn't work. The lisp vm is able to successfully
load the `bootstrap.lisp` file from the filesystem, but loading additional lisp files
currently appears to cause a panic. It's a work in progress.

For now, if you want to load lisp code into the VM, the easiest way is to put it into
the `lisp/bootstrap.lisp` file and then run `make populate_disk` in order to copy the
file to the kernel's disk. 

Of course, you can also always enter it in by hand.
