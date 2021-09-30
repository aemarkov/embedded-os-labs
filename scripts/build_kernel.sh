#!/bin/bash

source "$(dirname $0)/env.sh"

cd "$KERNEL_DIR"

if [ ! -f .config ]; then
    echo ".config file not found! Provide config!"
    exit 1
fi

# Change kernel name in config
cat .config | grep -E '^CONFIG_LOCALVERSION=.*-labs' > /dev/null
if [ $? -ne 0 ]; then
    sed -i -E 's/^CONFIG_LOCALVERSION="(.+)"/CONFIG_LOCALVERSION="\1-labs"/' .config
fi

set -e

echo "Updating config to new version"
echo "Please, press \"n <enter>\" for each question"
make ARCH=arm CROSS_COMPILE="$CROSS_COMPILE" oldconfig

echo "Building kernel..."
make ARCH=arm CROSS_COMPILE="$CROSS_COMPILE" -j $(nproc)

echo "Building image, modules, Device Tree"
make ARCH=arm CROSS_COMPILE="$CROSS_COMPILE" zImage modules dtbs -j $(nproc)

echo
echo "Done!"
