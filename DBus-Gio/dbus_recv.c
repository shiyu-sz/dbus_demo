#include <gio/gio.h>
#include <stdlib.h>

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

    // 订阅DBus服务名变化信号
    guint sub_id = g_dbus_connection_signal_subscribe(
        conn,
        "com.example.Service",       // 发送者
        "com.example.Interface",    // 接口名
        "MethodName",               // 信号名
        "/com/example/Service",      // 对象路径
        NULL,                         // 匹配规则（所有）
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_signal_received,
        NULL,
        NULL);

    g_print("Listening for service changes...\n");
}

static void on_name_appeared(GDBusConnection *conn, const gchar *name, const gchar *owner, gpointer data) {
    g_print("on_name_appeared : %s ( %s)\n", name, owner);
}

static void on_name_vanished(GDBusConnection *conn, const gchar *name, gpointer data) {
    g_print("on_name_vanished : %s\n", name);
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
