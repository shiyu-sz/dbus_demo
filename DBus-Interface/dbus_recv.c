#include <stdio.h>
#include "dbus_interface.h"
#include <dbus/dbus.h>
#include <stddef.h>

#define DBUS_RECEIVER_BUS "dbus_recv.receiver"

DBusConnection *dbus_recv_conn;

char *signal1_callback(char *signal, char *msg)
{
    printf("signal : %s, msg : %s\n", signal, msg);
    return NULL;
}

int main()
{
    if( dbus_recv_init(&dbus_recv_conn, DBUS_RECEIVER_BUS) != 0 )
    {
        printf("dbus_recv_init failed!");
        return -1;
    } else {
        dbus_add_signal_match(dbus_recv_conn, "signal1", signal1_callback);
    
        dbus_recv_loop(dbus_recv_conn);
    }

    return 0;
}