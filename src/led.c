/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2024 Deepak Kumar <notwho53@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libdroid/leds.h>
#include <libupower-glib/upower.h>
#include <glib-2.0/glib.h>

DroidLeds *leds;

static void on_battery_changed(UpDevice *device) {
    gdouble percentage;
    guint state;

    g_object_get(device,
                 "percentage", &percentage,
                 "state", &state,
                 NULL);

    // reset every time battery state/percentage gets changed
    droid_leds_set_notification(leds, 0xff000000, 0, 0);

    if (state == UP_DEVICE_STATE_CHARGING) {
        droid_leds_set_notification(leds, 0xffff0000, 0, 0);
    } else if (state == UP_DEVICE_STATE_FULLY_CHARGED) {
        droid_leds_set_notification(leds, 0xff00ff00, 0, 0);
    } else if (percentage < 20.0) {
        droid_leds_set_notification(leds, 0xffff0000, 1500, 1500);
    } else {
    }
}

int main() {
    UpClient *client = NULL;
    UpDevice *device = NULL;

    GError *err = NULL;
    leds = droid_leds_new(&err);

    client = up_client_new();
    if (client == NULL) {
        goto cleanup;
    }

    device = up_client_get_display_device(client);
    if (device == NULL) {
        goto cleanup;
    }

    g_signal_connect(device, "notify::percentage", G_CALLBACK(on_battery_changed), NULL);
    g_signal_connect(device, "notify::state", G_CALLBACK(on_battery_changed), NULL);

    on_battery_changed(device);

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

cleanup:
    if (device) {
        g_object_unref(device);
    }
    if (client) {
        g_object_unref(client);
    }
    if (leds) {
        droid_leds_set_notification(leds, 0xff000000, 0, 0);
        g_clear_error(&err);
        g_object_unref(leds);
    }

    return 0;
}
