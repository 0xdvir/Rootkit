#!/bin/bash

MOD_NAME="rootkit_module"
MOD_DEST_NAME="debug_netlinkx.ko"
MOD_DEST_DIR="/lib/modules/$(uname -r)/kernel/drivers/net"
MOD_DEST_PATH="$MOD_DEST_DIR/$MOD_DEST_NAME"

REC_SRC="userspace/build/receiver_helper"
HELPERS_DIR="/var/lib/.cache"
REC_PATH="$HELPERS_DIR/udevd-sync"

PERSISTENT="true"
SERVICE_NAME="ntp-update.service"
SERVICE_DIR="/etc/systemd/system"
SERVICE_PATH="$SERVICE_DIR/$SERVICE_NAME"

# Ensure destination directory exists
if [[ ! -d "$MOD_DEST_DIR" ]]; then
    echo "Destination directory $MOD_DEST_DIR does not exist."
    exit 2
fi

# Ensure destination directory exists
if [[ -d "$HELPERS_DIR" ]]; then
    rm -f $REC_PATH
fi

if [[ "$PERSISTENT" == "true" ]]; then
    if [[ -d "$SERVICE_DIR" ]]; then
        systemctl stop "$SERVICE_NAME" 2>/dev/null
        systemctl disable "$SERVICE_NAME" 2>/dev/null
        rm -f "$SERVICE_PATH"
        systemctl daemon-reload
    fi
fi

rmmod "$MOD_NAME" 2>/dev/null
rm -f "$MOD_DEST_PATH"
