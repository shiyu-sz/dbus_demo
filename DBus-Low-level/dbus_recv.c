#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DBusHandlerResult handle_message(DBusConnection* conn, DBusMessage* msg, void* data) {
    // 检查是否为方法调用
    if (dbus_message_is_method_call(msg, "com.example.Interface", "MethodName")) {
        DBusMessage* reply;
        char* param;

        // 提取参数
        if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &param, DBUS_TYPE_INVALID)) {
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
        printf("Received: %s\n", param);

        // 创建回复
        reply = dbus_message_new_method_return(msg);
        const char* response = "Hello from server!";
        dbus_message_append_args(reply, DBUS_TYPE_STRING, &response, DBUS_TYPE_INVALID);

        // 发送回复
        if (!dbus_connection_send(conn, reply, NULL)) {
            fprintf(stderr, "Failed to send reply\n");
        }
        dbus_connection_flush(conn);
        dbus_message_unref(reply);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

int main() {
    DBusConnection* conn;
    DBusError err;

    dbus_error_init(&err);
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Connection Error: %s\n", err.message);
        dbus_error_free(&err);
        return 1;
    }

    // 请求服务名
    int ret = dbus_bus_request_name(conn, "com.example.Service", 0, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Name Error: %s\n", err.message);
        dbus_error_free(&err);
        return 1;
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
        fprintf(stderr, "Not primary owner\n");
        return 1;
    }

    // 添加消息过滤器
    dbus_connection_add_filter(conn, handle_message, NULL, NULL);

    // 进入消息循环
    while (1) {
        dbus_connection_read_write_dispatch(conn, -1);
    }

    dbus_connection_unref(conn);
    return 0;
}
