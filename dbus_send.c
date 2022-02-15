
#include <stdio.h>
#include "dbus_interface.h"
#include <dbus/dbus.h>
#include <stddef.h>

#define DBUS_SENDER_BUS "dbus_send.sender"

DBusConnection *dbus_send_conn;

int main()
{
    if(dbus_send_init(&dbus_send_conn, DBUS_SENDER_BUS) != 0) 
    {
        printf("dbus_send_init failed!");
        return -1;
    }

    dbus_send_signal(dbus_send_conn, "signal1", "123");

    return 0;
}