#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libudev.h>
#include <libupower-glib/upower.h>
#include <glib-2.0/glib.h>
#include <errno.h>

#define DEBUG(fmt, args...) //fprintf(stderr, "DEBUG: " fmt "\n", ##args)

void set_led(struct udev *udev, const char *led_name, int value);
void stop_blinking();

typedef struct {
    struct udev *udev;
    gboolean led_state;
    guint timer_id;
} BlinkData;

static BlinkData *blink_data = NULL;

static gboolean blink_led_timeout(gpointer user_data) {
    BlinkData *data = (BlinkData *)user_data;

    if (data->led_state) {
        set_led(data->udev, "red", 0);
        data->timer_id = g_timeout_add_seconds(3, blink_led_timeout, user_data);
    } else {
        set_led(data->udev, "red", 1);
        data->timer_id = g_timeout_add_seconds(1, blink_led_timeout, user_data);
    }

    data->led_state = !data->led_state;

    return FALSE;
}

void blink_led(struct udev *udev) {
    DEBUG("Starting LED blink");
    if (blink_data) {
        stop_blinking();
    }
    blink_data = g_new0(BlinkData, 1);
    blink_data->udev = udev;
    blink_data->led_state = TRUE;
    blink_data->timer_id = g_timeout_add_seconds(1, blink_led_timeout, blink_data);
}

void stop_blinking() {
    if (blink_data) {
        if (blink_data->timer_id) {
            g_source_remove(blink_data->timer_id);
            blink_data->timer_id = 0;
        }
        set_led(blink_data->udev, "red", 0);
        g_free(blink_data);
        blink_data = NULL;
        DEBUG("Stopped LED blinking");
    }
}

void set_led(struct udev *udev, const char *led_name, int value) {
    DEBUG("Attempting to set LED %s to %d", led_name, value);

    struct udev_device *led_device = udev_device_new_from_subsystem_sysname(udev, "leds", led_name);
    if (!led_device) {
        DEBUG("Error finding LED device %s", led_name);
        return;
    }

    char value_str[4];
    snprintf(value_str, sizeof(value_str), "%d", value);

    if (udev_device_set_sysattr_value(led_device, "brightness", value_str) != 0) {
        DEBUG("Error setting brightness for LED %s", led_name);
    } else {
        DEBUG("Successfully set brightness of LED %s to %d", led_name, value);
    }

    udev_device_unref(led_device);
}

static void on_battery_changed(UpDevice *device, GParamSpec *pspec, gpointer user_data) {
    struct udev *udev = user_data;
    gdouble percentage;
    guint state;

    g_object_get(device,
                 "percentage", &percentage,
                 "state", &state,
                 NULL);

    DEBUG("Battery percentage: %.2f%%, State: %d", percentage, state);

    if (state == UP_DEVICE_STATE_CHARGING) {
        DEBUG("Battery is charging");
        stop_blinking();
        set_led(udev, "red", 1);
        set_led(udev, "green", 0);
    } else if (state == UP_DEVICE_STATE_FULLY_CHARGED) {
        DEBUG("Battery is fully charged");
        stop_blinking();
        set_led(udev, "red", 0);
        set_led(udev, "green", 1);
    } else if (percentage < 20.0) {
        DEBUG("Battery is low");
        if (!blink_data) {
            blink_led(udev);
        }
    } else {
        DEBUG("Battery is in normal state");
        stop_blinking();
        set_led(udev, "red", 0);
        set_led(udev, "green", 0);
    }
}

int main() {
    struct udev *udev = NULL;
    UpClient *client = NULL;
    UpDevice *device = NULL;

    DEBUG("Starting battery monitor program");

    udev = udev_new();
    if (!udev) {
        DEBUG("Failed to create udev");
        return 1;
    }
    DEBUG("udev initialized successfully");

    client = up_client_new();
    if (client == NULL) {
        DEBUG("Failed to create UPower client");
        goto cleanup;
    }
    DEBUG("UPower client created successfully");

    device = up_client_get_display_device(client);
    if (device == NULL) {
        DEBUG("Failed to get display device");
        goto cleanup;
    }
    DEBUG("Display device retrieved successfully");

    g_signal_connect(device, "notify::percentage", G_CALLBACK(on_battery_changed), udev);
    g_signal_connect(device, "notify::state", G_CALLBACK(on_battery_changed), udev);

    on_battery_changed(device, NULL, udev);

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

cleanup:
    DEBUG("Cleaning up resources");
    if (device) {
        g_object_unref(device);
        DEBUG("Device object unreferenced");
    }
    if (client) {
        g_object_unref(client);
        DEBUG("Client object unreferenced");
    }
    if (udev) {
        udev_unref(udev);
        DEBUG("udev unreferenced");
    }
    stop_blinking();

    return 0;
}
