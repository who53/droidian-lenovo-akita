#!/bin/bash

while true; do
    if [ -e /dev/wmtWifi ]; then
        echo 1 > /dev/wmtWifi
        exit 0
    fi
    sleep 3
done
