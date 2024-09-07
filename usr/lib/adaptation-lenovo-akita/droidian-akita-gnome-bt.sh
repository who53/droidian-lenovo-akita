#!/bin/bash

pid=$(WAYLAND_DISPLAY=wayland-0 gdbus call --session \
          --dest org.freedesktop.DBus \
          --object-path /org/freedesktop/DBus \
          --method org.freedesktop.DBus.GetConnectionUnixProcessID \
          "org.gnome.SettingsDaemon.Rfkill" | grep -oP '\d+' | sed -n '2p')

if [ -n "$pid" ]; then
    kill "$pid"
    echo "Killed process with PID: $pid"
else
    echo "Failed to extract PID"
    exit 1
fi

/usr/share/WHO53/gnome-bt &
