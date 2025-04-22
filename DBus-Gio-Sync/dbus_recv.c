#include <gio/gio.h>
#include <stdlib.h>

// 方法调用完成回调
static void on_method_call_complete(GObject      *source_object,
                                    GAsyncResult *res,
                                    gpointer      user_data)
{
    GError *error = NULL;
    GVariant *result = g_dbus_connection_call_finish(
        G_DBUS_CONNECTION(source_object), res, &error);

    if (error) {
        g_printerr("Method call failed: %s\n", error->message);
        g_error_free(error);
        return;
    }

    // 解析返回结果（ListNames返回字符串数组）
    GVariantIter *names;
    g_variant_get(result, "(^as)", &names);

    gchar *name;
    g_print("\n=== Method Call Result ===\n");
    while (g_variant_iter_next(names, "s", &name)) {
        g_print("Service: %s\n", name);
        g_free(name);
    }
    g_variant_iter_free(names);
    g_variant_unref(result);
}

// 信号接收回调
static void on_signal_received(GDBusConnection *connection,
                               const gchar     *sender_name,
                               const gchar     *object_path,
                               const gchar     *interface_name,
                               const gchar     *signal_name,
                               GVariant        *parameters,
                               gpointer         user_data)
{
    // 解析NameOwnerChanged信号参数（三个字符串）
    const gchar *name, *old_owner, *new_owner;
    g_variant_get(parameters, "(&s&s&s)", &name, &old_owner, &new_owner);
    
    g_print("\n=== Signal Received ===\n"
            "Signal: %s\n"
            "Service: %s\n"
            "Old owner: %s\n"
            "New owner: %s\n",
            signal_name, name, old_owner, new_owner);
}

// DBus连接建立回调
static void on_bus_acquired(GObject      *source_object,
                            GAsyncResult *res,
                            gpointer      user_data)
{
    GError *error = NULL;
    GDBusConnection *conn = g_bus_get_finish(res, &error);
    if (!conn) {
        g_printerr("Failed to connect to DBus: %s\n", error->message);
        g_error_free(error);
        exit(1);
    }

    // 发起异步方法调用（获取所有已注册的服务名）
    g_dbus_connection_call(conn,
        "org.freedesktop.DBus",       // 目标服务名
        "/org/freedesktop/DBus",      // 对象路径
        "org.freedesktop.DBus",       // 接口名
        "ListNames",                  // 方法名
        NULL,                         // 输入参数（无）
        NULL,                         // 输出参数类型（自动推导）
        G_DBUS_CALL_FLAGS_NONE,
        -1,                           // 超时（无）
        NULL,                         // GCancellable
        on_method_call_complete,
        NULL);

    // 订阅DBus服务名变化信号
    guint sub_id = g_dbus_connection_signal_subscribe(
        conn,
        "org.freedesktop.DBus",       // 发送者
        "org.freedesktop.DBus",       // 接口名
        "NameOwnerChanged",          // 信号名
        "/org/freedesktop/DBus",      // 对象路径
        NULL,                         // 匹配规则（所有）
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_signal_received,
        NULL,
        NULL);

    g_print("Listening for service changes...\n");
}

int main()
{
    GMainLoop *loop;

    // 初始化线程系统（GLib需要）
    g_type_init();

    // 异步获取会话总线连接
    g_bus_get(G_BUS_TYPE_SESSION,
              NULL,                   // GCancellable
              on_bus_acquired,
              NULL);

    // 启动主事件循环
    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    // 清理（通常不会执行到这里）
    g_main_loop_unref(loop);
    return 0;
}
