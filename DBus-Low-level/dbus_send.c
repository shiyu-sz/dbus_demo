#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    DBusConnection* conn = NULL;
    DBusMessage* msg = NULL;
    DBusPendingCall* pending = NULL;
    DBusError err;

    // 初始化错误
    dbus_error_init(&err);
    
    // 建立会话总线连接
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Connection Error: %s\n", err.message);
        dbus_error_free(&err);
        exit(1);
    }

    // 创建方法调用消息
    msg = dbus_message_new_method_call(
        "com.example.Service",       // 目标服务名
        "/com/example/Service",      // 对象路径
        "com.example.Interface",     // 接口名
        "MethodName");              // 方法名

    if (!msg) {
        fprintf(stderr, "Message Null\n");
        exit(1);
    }

    // 添加参数（示例字符串参数）
    const char* param = "Hello from client!";
    if (!dbus_message_append_args(msg, DBUS_TYPE_STRING, &param, DBUS_TYPE_INVALID)) {
        fprintf(stderr, "Out Of Memory!\n");
        exit(1);
    }

    // 发送消息并等待回复
    if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) { // -1为默认超时
        fprintf(stderr, "Out Of Memory!\n");
        exit(1);
    }
    
    dbus_connection_flush(conn);
    dbus_message_unref(msg);

    // 阻塞等待回复
    dbus_pending_call_block(pending);
    msg = dbus_pending_call_steal_reply(pending);
    if (!msg) {
        fprintf(stderr, "Reply Null\n");
        exit(1);
    }
    dbus_pending_call_unref(pending);

    // 读取回复参数
    char* reply;
    if (!dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &reply, DBUS_TYPE_INVALID)) {
        fprintf(stderr, "Failed to get reply: %s\n", err.message);
        dbus_error_free(&err);
        exit(1);
    }
    printf("Reply: %s\n", reply);

    // 清理
    dbus_message_unref(msg);
    dbus_connection_unref(conn);
    return 0;
}
