#!/bin/bash

# Script to set environment variables

# Get current script path. Works for bash and zsh. Maybe, works for other shells
SCRIPT="${BASH_SOURCE[0]:-$0}"

# Directories
export SCRIPTS_DIR="$(realpath "$(dirname "$SCRIPT")")"
export BASE_DIR="$(realpath "$SCRIPTS_DIR/..")"
export DIST_DIR="$BASE_DIR/dist"
export TOOLS_DIR="$DIST_DIR/tools"
export KERNEL_DIR="$DIST_DIR/linux"

# Variables to build Linux kernel
TOOLCHAIN_DIR="toolchain/bin"
TOOLCHAIN_PREFIX="arm-none-linux-gnueabihf-"
export CROSS_COMPILE="$TOOLS_DIR/$TOOLCHAIN_DIR/$TOOLCHAIN_PREFIX"
export KERNEL=kernel7
