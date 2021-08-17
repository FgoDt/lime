#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <dbus/dbus.h>
#include "Registrar.h"

const char * server_name = "com.canonical.AppMenu.Registrar";

static void 
on_bus_acquired(GDBusConnection *con, const gchar *name, gpointer udata)
{
    g_print("on acquired %s\n",name);
    GError *err= NULL;
}

static gboolean handle_register_window(Registrar* skeleton,
    GDBusMethodInvocation *invocation,
    guint32 *windowid, gchar* path, gpointer *udata){

        printf("in register window\n");

}

static void
on_name_acquired(GDBusConnection *con, const gchar *name, gpointer udata)
{
    GError *error = NULL;
    Registrar* skeleton;
    skeleton = registrar_skeleton_new();
    g_signal_connect(skeleton,"handle-register-window",G_CALLBACK(handle_register_window),NULL);
    g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(skeleton), con,
     "/com/canonical/AppMenu/Registrar",&error);
    if(error != NULL){
        g_print("error on skeleton export %s\n", error->message);
    }
    g_print("on name acquired %s\n",name);
}

static void
on_name_lost(GDBusConnection *con, const gchar *name, gpointer udata)
{
    g_print("on name lost\n");
}


int main(){

    DBusError err;

    GError *error = NULL;


    guint owner_id;


    if(error != NULL){
        printf("error\n");
    }


    owner_id = g_bus_own_name(G_BUS_TYPE_SESSION,
                    server_name,
                    G_BUS_NAME_OWNER_FLAGS_NONE,
                    on_bus_acquired,
                    on_name_acquired,
                    on_name_lost,
                    NULL,
                    NULL
                    );
  
    
    GMainLoop* loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
    g_bus_unown_name(owner_id);

    return 0;
}
