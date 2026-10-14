#!/usr/bin/env bash

gattlib_dir=${GATTLIB_DIR:-"$HOME/Lab3/gattlib"}
char_uuid='00002222-0000-1000-8000-00805f9b34fb'
rw="$gattlib_dir/build/examples/read_write/read_write"

sudo -v

# Scan for 10 seconds and display Lab3_test advertisements.
echo '--- Scan ---'
sudo timeout 10s stdbuf -o0 -e0 \
    "$gattlib_dir/build/examples/advertisement_data/advertisement_data" 2>&1 |
    tee gattlib-scan.log |
    grep --line-buffered -E 'Lab3_test|[Ee]rror|[Ff]ail'

# Use the first address advertising the exact name Lab3_test.
phone_ble_addr=$(grep -oE "Device ([[:xdigit:]]{2}:){5}[[:xdigit:]]{2} - 'Lab3_test':" \
    gattlib-scan.log | head -n 1 | cut -d ' ' -f 2)
echo "phone_ble_addr=$phone_ble_addr"

echo '--- Read ---'
sudo timeout 10s stdbuf -oL -eL "$rw" "$phone_ble_addr" read "$char_uuid" 2>&1 |
    tee gattlib-read.log |
    grep --line-buffered -vE '^(DBUS:|gattlib_device_set_state:)'

read -r -p 'Value to write (hex, e.g. 0x1234): ' write_value
echo "--- Write $write_value ---"
sudo timeout 10s stdbuf -oL -eL "$rw" "$phone_ble_addr" write "$char_uuid" "$write_value" 2>&1 |
    tee gattlib-write.log |
    grep --line-buffered -vE '^(DBUS:|gattlib_device_set_state:)'

echo '--- Read-back ---'
sudo timeout 10s stdbuf -oL -eL "$rw" "$phone_ble_addr" read "$char_uuid" 2>&1 |
    tee gattlib-readback.log |
    grep --line-buffered -vE '^(DBUS:|gattlib_device_set_state:)'

