#!/bin/bash

#get variables
export LINUX_OVERRIDE_SRCDIR=~/Downloads/linuxdistro/linux-4.13.9/

#build updates
make

#run qemu
sudo qemu-system-i386 --kernel output/images/bzImage --hda output/images/rootfs.ext2 --nographic --append "console=ttyS0 root=/dev/sda"
