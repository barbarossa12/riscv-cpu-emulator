#!/bin/sh
# Run by buildroot on the target tree before the cpio is made. $1 is the target directory.
# The kernel has no network stack (CONFIG_NET is off), so the network start-up script can only fail.
set -e
rm -f "$1/etc/init.d/S40network"
