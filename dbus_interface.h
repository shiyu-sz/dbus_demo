#ifndef DBUS_INTERFACE_H
#define DBUS_INTERFACE_H

#include <dbus/dbus.h>

/**
 * dbus发送初始化
 */
int dbus_send_init(DBusConnection **conn, const char* bus_name);

/**
 * dbus接收初始化
 */
int dbus_recv_init(DBusConnection **conn, const char* bus_name);

/**
 * dbus添加异步消息监听
 */

int dbus_add_signal_match(DBusConnection *conn, const char *signal, char *(*callback)(char *name, char *msg));

/**
 * dbus添加同步消息监听
 */
int dbus_add_method_match(DBusConnection *conn, const char *method, char *(*callback)(char *name, char *msg));

/**
 * dbus接收监听，放到一个单独的线程中
 */
int dbus_recv_loop(DBusConnection *conn);

/**
 * dbus发送signal
 * conn : dbus句柄
 * signal ： 信号
 * message ： 消息
 */
int dbus_send_signal(DBusConnection *conn,  const char *signal, const char* message);

/**
 * dbus同步发送
 */
int dbus_query(DBusConnection *conn, const char* target_bus, const char* method, const char* message, char **data);

/**
 * dbus释放内存
 */
void dbus_release_message(char *data);

#endif
