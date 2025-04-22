#include <gio/gio.h>
#include <stdio.h>

int main() {
    GError *error = NULL;
    GDBusConnection *conn;
    GVariant *result;

    // 连接到会话总线
    conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (error) {
        g_printerr("连接错误: %s\n", error->message);
        g_error_free(error);
        return 1;
    }

    // 调用DBus方法
    result = g_dbus_connection_call_sync(conn,
        "com.example.Service",          // 目标服务名
        "/com/example/Service",         // 对象路径
        "com.example.Interface",          // 接口名
        "MethodName",                       // 方法名
        g_variant_new("(s)", "Hello from C"), // 参数
        G_VARIANT_TYPE("(s)"),            // 返回类型
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);

    if (error) {
        g_printerr("调用错误: %s\n", error->message);
        g_error_free(error);
        return 1;
    }

    // 解析返回结果
    const gchar *response;
    g_variant_get(result, "(s)", &response);
    g_print("recv : %s\n", response);

    // 清理资源
    g_variant_unref(result);
    g_object_unref(conn);
    return 0;
}