#!/bin/bash

# This script downloads Linux kernel for Raspberry Pi

source "$(dirname $0)/env.sh"

URL="https://github.com/raspberrypi/linux"

set -e

mkdir -p "$KERNEL_DIR"

echo "Downloading Linux Kernel..."
git clone "$URL" "$KERNEL_DIR" --depth 1
echo
echo "Kernel is downloaded to the $TOOLS_DIR"
