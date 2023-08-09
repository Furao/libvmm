# A simple VMM for running Linux guests

This example is a minimal VMM that supports Linux guests and a basic
buildroot/BusyBox root file system. This gives a basic command-line with some
utilities.

The example currently works on the following platforms:
* Hardkernel Odroid-C4
* QEMU ARM virt

## Building with Make

```sh
make BOARD=<BOARD> SEL4CP_SDK=/path/to/sel4cp-sdk-1.2.6
```

Where `<BOARD>` is one of:
* `qemu_arm_virt_hyp`
* `odroidc4_hyp`

Other configuration options can be passed to the Makefile such as `CONFIG`
and `BUILD_DIR`, see the Makefile for details.

If you would like to simulate the QEMU board you can run the following command:
```sh
make BOARD=qemu_arm_virt_hyp SEL4CP_SDK=/path/to/sel4cp-sdk-1.2.6 qemu
```

This will build the example code as well as run the QEMU command to simulate a
system running the whole system.

## Building with Zig

For educational purposes, you can also build and run this example using the
[Zig](https://ziglang.org/) build system.

You will first need Zig version 0.11.0, which can be downloaded here: [TODO]().

```sh
# TODO @ivanv fix this command to also include board
zig build -Dtarget="aarch64-freestanding" -Doptimize=ReleaseFast qemu
```

[A known issue]() is that compiling the example without optimisations does not
cause the example to function correctly.