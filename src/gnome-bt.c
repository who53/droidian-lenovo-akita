#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>

#define DEBUG(fmt, args...) //fprintf(stderr, "DEBUG: " fmt "\n", ##args)

static GDBusNodeInfo *introspection_data = NULL;
static gboolean bluetooth_airplane_mode = FALSE;

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.gnome.SettingsDaemon.Rfkill'>"
  "    <property name='AirplaneMode' type='b' access='readwrite'/>"
  "    <property name='HardwareAirplaneMode' type='b' access='read'/>"
  "    <property name='BluetoothAirplaneMode' type='b' access='readwrite'/>"
  "    <property name='BluetoothHasAirplaneMode' type='b' access='read'/>"
  "    <property name='BluetoothHardwareAirplaneMode' type='b' access='readwrite'/>"
  "    <property name='ShouldShowAirplaneMode' type='b' access='read'/>"
  "  </interface>"
  "</node>";

static void
handle_method_call (GDBusConnection       *connection,
                    const gchar           *sender,
                    const gchar           *object_path,
                    const gchar           *interface_name,
                    const gchar           *method_name,
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer               user_data)
{
  g_dbus_method_invocation_return_error (invocation,
                                         G_DBUS_ERROR,
                                         G_DBUS_ERROR_UNKNOWN_METHOD,
                                         "Method %s.%s not implemented",
                                         interface_name,
                                         method_name);
}

static void
toggle_bluetooth(gboolean enable)
{
    if (enable) {
        DEBUG("Turning Bluetooth ON\n");
         system("bluetoothctl power on");
    } else {
        DEBUG("Turning Bluetooth OFF\n");
         system("bluetoothctl power off");
    }
}

static GVariant *
handle_get_property (GDBusConnection  *connection,
                     const gchar      *sender,
                     const gchar      *object_path,
                     const gchar      *interface_name,
                     const gchar      *property_name,
                     GError          **error,
                     gpointer          user_data)
{
  if (g_strcmp0 (property_name, "BluetoothHasAirplaneMode") == 0)
  {
    return g_variant_new_boolean (TRUE);
  }
  else if (g_strcmp0 (property_name, "BluetoothAirplaneMode") == 0)
  {
    return g_variant_new_boolean (bluetooth_airplane_mode);
  }
  else
  {
    return g_variant_new_boolean (FALSE);
  }
}

static gboolean
handle_set_property (GDBusConnection  *connection,
                     const gchar      *sender,
                     const gchar      *object_path,
                     const gchar      *interface_name,
                     const gchar      *property_name,
                     GVariant         *value,
                     GError          **error,
                     gpointer          user_data)
{
  if (g_strcmp0 (property_name, "BluetoothAirplaneMode") == 0)
  {
    gboolean new_value;
    g_variant_get(value, "b", &new_value);
    
    if (new_value != bluetooth_airplane_mode)
    {
      bluetooth_airplane_mode = new_value;
      toggle_bluetooth(!bluetooth_airplane_mode);
      
      GVariantBuilder *builder;
      GVariant *changed_properties;
      GVariantBuilder *invalidated_builder;
      
      builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
      g_variant_builder_add (builder,
                             "{sv}",
                             "BluetoothAirplaneMode",
                             g_variant_new_boolean (bluetooth_airplane_mode));
      changed_properties = g_variant_builder_end (builder);
      g_variant_builder_unref (builder);
      
      invalidated_builder = g_variant_builder_new (G_VARIANT_TYPE("as"));
      GVariant *invalidated_properties = g_variant_builder_end (invalidated_builder);
      g_variant_builder_unref (invalidated_builder);
      
      g_dbus_connection_emit_signal (connection,
                                     NULL,
                                     "/org/gnome/SettingsDaemon/Rfkill",
                                     "org.freedesktop.DBus.Properties",
                                     "PropertiesChanged",
                                     g_variant_new ("(s@a{sv}@as)",
                                                    "org.gnome.SettingsDaemon.Rfkill",
                                                    changed_properties,
                                                    invalidated_properties),
                                     NULL);
    }
    return TRUE;
  }
  else
  {
    g_set_error (error,
                 G_DBUS_ERROR,
                 G_DBUS_ERROR_INVALID_ARGS,
                 "Setting property %s not allowed",
                 property_name);
    return FALSE;
  }
}

static const GDBusInterfaceVTable interface_vtable =
{
  handle_method_call,
  handle_get_property,
  handle_set_property
};

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
  guint registration_id;

  registration_id = g_dbus_connection_register_object (connection,
                                                       "/org/gnome/SettingsDaemon/Rfkill",
                                                       introspection_data->interfaces[0],
                                                       &interface_vtable,
                                                       NULL,  /* user_data */
                                                       NULL,  /* user_data_free_func */
                                                       NULL); /* GError** */
  g_assert (registration_id > 0);
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
  DEBUG ("Acquired the name %s on the session bus\n", name);
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
  DEBUG ("Lost the name %s on the session bus\n", name);
  exit (1);
}

int
main (int argc, char *argv[])
{
  guint owner_id;
  GMainLoop *loop;

  introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  g_assert (introspection_data != NULL);

  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             "org.gnome.SettingsDaemon.Rfkill",
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             on_bus_acquired,
                             on_name_acquired,
                             on_name_lost,
                             NULL,
                             NULL);

  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);

  g_bus_unown_name (owner_id);
  g_dbus_node_info_unref (introspection_data);
  g_main_loop_unref (loop);

  return 0;
}
