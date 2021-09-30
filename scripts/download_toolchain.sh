#!/bin/bash

# This script downloads toolchain for Raspberry Pi
# TODO: Can't build with this toolchain

source "$(dirname $0)/env.sh"

URL="https://developer.arm.com/-/media/Files/downloads/gnu-a/10.3-2021.07/binrel/gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf.tar.asc"
FILE="toolchain.tar.gz"
TOOLCHAIN_DIR="toolchain"

set -e

mkdir -p "$TOOLS_DIR"
cd "$TOOLS_DIR"

echo "Downloading ARM toolchain..."
wget "$URL" -O "$FILE"
echo "Extracting toolchain..."
tar -xvf "$FILE"
mv gcc-arm-*-arm-none-linux-gnueabihf "$TOOLS_DIR"
echo
echo "Toolchain is downloaded to the $TOOLS_DIR/$TOOLS_DIR"
