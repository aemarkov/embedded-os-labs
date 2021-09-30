#!/bin/bash

BOOT_DIR="$1"
ROOTFS_DIR="$2"
KERNEL_FILE="kernel-labs.img"

if [ -z "$BOOT_DIR" ] || [ -z "$ROOTFS_DIR" ]; then
    echo "Invalud arguments"
    echo "Usage:"
    echo "install_kernel_rpi.sh <path to boot> <path to rootfs>"
    exit 1
fi

if [ ! -d "$BOOT_DIR" ]; then
    echo "Directory doesn't exist: $BOOT_DIR"
    exit 1
fi
if [ ! -d "$ROOTFS_DIR" ]; then
    echo "Directory doesn't exist: $ROOTFS_DIR"
    exit 1
fi

source "$(dirname $0)/env.sh"

cd "$KERNEL_DIR"

set -e


echo "Copying kernel..."
sudo cp arch/arm/boot/zImage "$BOOT_DIR/$KERNEL_FILE"

echo "Copying device tree..."
sudo mkdir -p "$BOOT_DIR/overlays"
sudo cp arch/arm/boot/dts/*.dtb "$BOOT_DIR/"
sudo cp arch/arm/boot/dts/overlays/*.dtb "$BOOT_DIR/overlays/"
sudo cp arch/arm/boot/dts/overlays/README "$BOOT_DIR/overlays/"

echo "Copying modules..."
sudo env PATH=$PATH make ARCH=arm CROSS_COMPILE="$CROSS_COMPILE" \
    INSTALL_MOD_PATH="$ROOTFS_DIR" modules_install

# Modify config to run custom kernel
sed -i "1 i\kernel=$KERNEL_FILE" "$BOOT_DIR/config.txt"

echo "Syncing..."
sync
