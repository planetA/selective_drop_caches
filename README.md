selective_drop_caches
=====================

Drops caches by path.

Example:

echo ./ > /proc/sys/vm/sdrop_caches

Makefile short guide:
=====================
Set up 	KERNEL_SRC variable to perform compilation.

There exist following making targets
------------------------------------

  all: compiles module

  clean: cleans working directory

  deliver: copies module to virtual machine

  kernel_deliver: copies kernel image to virtual machine. Assumes that VM is already configured properly (grub.cfg has appropriate entry, initrd is already generated)

  qemu: Starts kvm VM. Assumes that image is in ../debian.img.

  ssh: Connects to VM via SSH

  insmod: Insert module (should be already copied).

  rmmod: Removes module.

  gdb: Insert module and starts gdb with module symbol file loaded. Removes module upon exiting the gdb.

  reboot: Reboots VM via ssh.
