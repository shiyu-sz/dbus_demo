#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <dbus/dbus.h>
#include <libubox/list.h>

#define DBUS_MSG_MAX    (32*1024)
#define DBUS_QUERY_MSG_MAX  256

#define DBUS_SENDER_BUS_NAME        "test.signal.source"
#define DBUS_RECEIVER_BUS_NAME      "test.signal.sink"
#define DBUS_QUERY_BUS_NAME         "test.method.caller"
#define DBUS_REPLY_BUS_NAME         "test.method.server"

#define DBUS_METHOD_TARGET          "%s.method.target"
#define DBUS_METHOD_OBJECT        	"/%s/method/object"
#define DBUS_METHOD_INTERFACE     	"%s.method.type"

#define DBUS_OBJECT        	"/%s/signal/object"
#define DBUS_INTERFACE     	"%s.signal.type"
#define DBUS_MATCH  		"type='signal',interface='%s.signal.type'"
#define DBUS_METHOD_MATCH   "member='%s'"

#define MAX_WATCHES 100

static struct pollfd pollfds[MAX_WATCHES];
static DBusWatch *watches[MAX_WATCHES];
static int max_i;

struct match_list_t {
    char name[64];
    char *(*callback)(char *name, char *msg);
    struct list_head list;
};

struct match_list_t signal_match_list;
struct match_list_t method_match_list;

void reply_to_method_call(DBusConnection* conn, DBusMessage *msg, char *data)
{
    printf("%s\n", __FUNCTION__);
    DBusMessage* reply;
    DBusMessageIter args;
    dbus_uint32_t serial = 0;
    char* param = "";

    // read the arguments
    if (!dbus_message_iter_init(msg, &args))
        printf("Message has no arguments!\n"); 
    else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args)) 
        printf("Argument is not string!\n"); 
    else 
        dbus_message_iter_get_basic(&args, &param);

    // printf("Method called with %s\n", param);
    // create a reply from the message
    reply = dbus_message_new_method_return(msg);
    if( data != NULL ) {
        // add the arguments to the reply
        dbus_message_iter_init_append(reply, &args);
        printf("%s 11\n", __FUNCTION__);
        if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &data)) { 
            printf("Out Of Memory!\n"); 
            return;
        }
    }
    // send the reply && flush the connection
    if (!dbus_connection_send(conn, reply, &serial)) {
        printf("Out Of Memory!\n"); 
        return;
    }
    dbus_connection_flush(conn);
    // free the reply
    dbus_message_unref(reply);
    if( data != NULL ) {
        free(data);
    }
}

#define DBUS_TYPE_MESSAGE 4
#define DBUS_TYPE_METHOD  1
static DBusHandlerResult filter_func(DBusConnection *conn, DBusMessage *msg, void *data)
{
    char* sigvalue = NULL;
    char *call_retuen = NULL;
    DBusMessageIter args;
	struct match_list_t *pos = NULL;

	char *interface_str = dbus_message_get_interface(msg);
    char *member_str = dbus_message_get_member(msg);
    int type = dbus_message_get_type(msg);

    if ((interface_str == NULL) || (member_str == NULL))
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	printf("(dbus)dbus recv = interface:%s, member:%s, path:%s, type=%d\n",
            dbus_message_get_interface(msg),	// interface
            dbus_message_get_member(msg),	// signal
            dbus_message_get_path(msg),     // object
            type);	// type

    if (type == DBUS_TYPE_METHOD) {
        list_for_each_entry(pos, &method_match_list.list, list)
        {
            printf("pos->name = %s \n", pos->name);
            if (strcmp(member_str, pos->name) == 0)
            {
                printf("(dbus)find match method!\n");
                if (dbus_message_is_method_call(msg, interface_str, member_str))
                {
                    // read the parameters
                    if (!dbus_message_iter_init(msg, &args))
                        printf("(dbus)%s:%d Message Has No Parameters\n", __FUNCTION__, __LINE__);
                    else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args))
                        printf("(dbus)Argument is not string!\n");
                    else
                        dbus_message_iter_get_basic(&args, &sigvalue);
                    if( sigvalue != NULL )
                        printf("(dbus)dbus recv method = %s, message = %s\n", member_str, sigvalue);
                    if( pos->callback != NULL ) {
                        call_retuen = (char *)pos->callback(member_str, sigvalue);
                    }
                    reply_to_method_call(conn, msg, call_retuen);
                }
            }
        }
    } else if (type == DBUS_TYPE_MESSAGE) {
        list_for_each_entry(pos, &signal_match_list.list, list)
        {
            // printf("pos->signal = %s \n", pos->signal);
            if (strcmp(member_str, pos->name) == 0)
            {
                if (dbus_message_is_signal(msg, interface_str, member_str))
                {
                    // read the parameters
                    if (!dbus_message_iter_init(msg, &args))
                        printf("(dbus)%s:%d Message Has No Parameters\n", __FUNCTION__, __LINE__);
                    else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args))
                        printf("(dbus)Argument is not string!\n");
                    else
                        dbus_message_iter_get_basic(&args, &sigvalue);
                    if( sigvalue != NULL )
                        printf("(dbus)dbus recv signal = %s message = %s\n", member_str, sigvalue);
                    if( pos->callback != NULL ) {
                        pos->callback(member_str, sigvalue);
                    }
                }
            }
        }
    }

	return DBUS_HANDLER_RESULT_HANDLED;
}



static void fd_handler(DBusConnection *conn, short events, DBusWatch *watch)
{
	unsigned int flags = 0;

	if(events & POLLIN)
		flags |= DBUS_WATCH_READABLE;
	if(events & POLLOUT)
		flags |= DBUS_WATCH_WRITABLE;
	if(events & POLLHUP)
		flags |= DBUS_WATCH_HANGUP;
	if(events & POLLERR)
		flags |= DBUS_WATCH_ERROR;

	while(!dbus_watch_handle(watch, flags)) {
		printf("(dbus)dbus_watch_handle needs more memory\n");
		sleep(1);
	}
	dbus_connection_ref(conn);
	while(dbus_connection_dispatch(conn) == DBUS_DISPATCH_DATA_REMAINS);
	dbus_connection_unref(conn);
}



static dbus_bool_t add_watch(DBusWatch *watch, void *data)
{
	short cond = POLLHUP | POLLERR;
	int fd;
	unsigned int flags;

	printf("(dbus)dbus add watch %p\n", (void *)watch);
	fd = dbus_watch_get_unix_fd(watch);
	flags = dbus_watch_get_flags(watch);

	if(flags & DBUS_WATCH_READABLE)
		cond |= POLLIN;
	if(flags & DBUS_WATCH_WRITABLE)
		cond |= POLLOUT;
	++max_i;
	pollfds[max_i].fd = fd;
	pollfds[max_i].events = cond;
	watches[max_i] = watch;

	return 1;
}

static void remove_watch(DBusWatch *watch, void *data)
{
	int i, found = 0;

	printf("(dbus)dbus remove watch %p/n", (void*)watch);

	for(i=0; i<=max_i;i++) {
		if(watches[i] == watch) {
			found = 1;
			break;
		}
	}
	if(!found) {
		printf("(dbus)watch %p not found!\n", (void *)watch);
		return ;
	}

	memset(&pollfds[i], 0, sizeof(pollfds[i]));
	watches[i] = NULL;
	if(i == max_i && max_i > 0)
		--max_i;
}

/**
 * dbus接收初始化
 */
int dbus_recv_init(DBusConnection **conn, const char* bus_name)
{
	DBusError err;
    int ret;

	printf("%s", __FUNCTION__);

    // initialise the errors
    dbus_error_init(&err);
    // connect to the bus and check for errors
    *conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) {
        printf("(dbus)Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (NULL == conn) {
        printf("(dbus)sdbus_bus_get Error!\n");
    }

    // request our name on the bus and check for errors
    ret = dbus_bus_request_name(*conn, bus_name, DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
    if (dbus_error_is_set(&err)) {
        printf("(dbus)Name Error (%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
        printf("(dbus)dbus_bus_request_name ret = %d\n", ret);
    }

	if(!dbus_connection_set_watch_functions(*conn, add_watch, remove_watch, NULL, NULL, NULL)) {
		printf("(dbus)dbus_connection_set_watch_functions failed\n");
		dbus_error_free(&err);
		return -1;
	}

	if(!dbus_connection_add_filter(*conn, filter_func, NULL, NULL)) {
		printf("(dbus)dbus_connection_add_filter failed!\n");
		dbus_error_free(&err);
		return -1;
	}

	INIT_LIST_HEAD(&signal_match_list.list);
    INIT_LIST_HEAD(&method_match_list.list);
	return 0;
}

/**
 * dbus添加消息监听
 */

int dbus_add_signal_match(DBusConnection *conn, const char *signal, char *(*callback)(char *name, char *msg))
{
    printf("%s", __FUNCTION__);

    struct match_list_t *tmp = NULL;
    tmp = (struct match_list_t *)malloc(sizeof(struct match_list_t));
    if (tmp)
    {
        strncpy(tmp->name, signal, 64);
        tmp->callback = callback;
        list_add(&(tmp->list), &signal_match_list.list);
    }

    char match_str[128];
    memset(match_str, 0, 128);
    snprintf(match_str, 128, DBUS_MATCH, signal);
    printf("(dbus)match_str = %s", match_str);
    dbus_bus_add_match(conn, match_str, NULL);

    return 0;
}

int dbus_add_method_match(DBusConnection *conn, const char *method, char *(*callback)(char *name, char *msg))
{
    printf("%s", __FUNCTION__);

    struct match_list_t *tmp = NULL;
    tmp = (struct match_list_t *)malloc(sizeof(struct match_list_t));
    if (tmp)
    {
        strncpy(tmp->name, method, 64);
        tmp->callback = callback;
        list_add(&(tmp->list), &method_match_list.list);
    }

    char match_str[128];
    memset(match_str, 0, 128);
    snprintf(match_str, 128, DBUS_METHOD_MATCH, method);
    printf("(dbus)match_str = %s", match_str);
    dbus_bus_add_match(conn, match_str, NULL);

    return 0;
}

/**
 * dbus发送初始化
 */
int dbus_send_init(DBusConnection **conn, const char* bus_name)
{
    printf("(dbus)%s", __FUNCTION__);
    DBusError err;
    int ret;

    // initialise the error value
   dbus_error_init(&err);

   // connect to the DBUS system bus, and check for errors
   *conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
   if (dbus_error_is_set(&err)) {
        printf("(dbus)Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
        return -1;
   }
   if (NULL == conn) {
        printf("(dbus)dbus_bus_get is null!\n");
        return -1;
   }

    // register our name on the bus, and check for errors
    ret = dbus_bus_request_name(*conn, bus_name, DBUS_NAME_FLAG_ALLOW_REPLACEMENT|DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
    if (dbus_error_is_set(&err)) {
      	printf("(dbus)Name Error (%s)\n", err.message);
      	dbus_error_free(&err);
        return -1;
    }

    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
		printf("(dbus)DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER ret = %d\n", ret);
        return -1;
    }

    return 0;
}

int dbus_query(DBusConnection *conn, const char* target_bus, const char* method, const char* message, char **data)
{
    char* sigvalue = NULL;
    DBusMessage* msg = NULL;
    DBusMessageIter args;
    DBusMessageIter args_recv;
    DBusPendingCall* pending = NULL;
	char object_str[DBUS_QUERY_MSG_MAX];
	char interface_str[DBUS_QUERY_MSG_MAX];

    if( message == NULL )
    {
        printf("(dbus)error message is null!\n");
        return -1;
    }

    memset(object_str, 0, DBUS_QUERY_MSG_MAX);
    memset(interface_str, 0, DBUS_QUERY_MSG_MAX);

	snprintf(object_str, DBUS_QUERY_MSG_MAX, DBUS_METHOD_OBJECT, method);
	snprintf(interface_str, DBUS_QUERY_MSG_MAX, DBUS_METHOD_INTERFACE, method);

    if(message != NULL) {
        printf("(dbus)%s, object_str = %s, interface_str = %s, signal = %s, message = %s",
            __FUNCTION__, object_str, interface_str, method, message);
    }

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(target_bus, // target for the method call
                                      object_str, // object to call on
                                      interface_str, // interface to call on
                                      method); // method name

    if (NULL == msg) { 
        printf("(dbus)Message Null\n");
        return -1;
    }

    // append arguments
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &message)) {
        printf("(dbus)Out Of Memory!\n");
        return -1;
    }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, -1)) { // -1 is default timeout
        printf("(dbus)Out Of Memory!\n"); 
        return -1;
    }

    if (NULL == pending) { 
        printf("(dbus)Pending Call Null\n"); 
        return -1; 
    }

    dbus_connection_flush(conn);

    // free message
    dbus_message_unref(msg);
    printf("(dbus) start block until reply!\n");

    // block until we recieve a reply
    dbus_pending_call_block(pending);
    printf("(dbus) recv reply!\n");

    // get the reply message
    msg = dbus_pending_call_steal_reply(pending);
    if (NULL == msg) {
        printf("Reply Null\n");
        return 0;
    }

    // read the parameters
    if (!dbus_message_iter_init(msg, &args_recv))
        printf("(dbus)%s:%d Message Has No Parameters\n", __FUNCTION__, __LINE__);
    else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args_recv))
        printf("(dbus)Argument is not string!\n");
    else
        dbus_message_iter_get_basic(&args_recv, &sigvalue);
    if(sigvalue != NULL)
    {
        printf("(dbus) %s:%d strlen", __FUNCTION__, __LINE__);
        int len = strlen(sigvalue);
        if( len + 1> DBUS_MSG_MAX) {
            len = DBUS_MSG_MAX - 1;
        }
        printf("(dbus) %s:%d malloc", __FUNCTION__, __LINE__);
        char *tmp = (char *)malloc(len + 1);
        if(tmp != NULL) {
            memcpy(tmp, sigvalue, len);
            tmp[len] = '\0';
            *data = tmp;
        }else {
            *data = NULL;
        }
    }
    else{
        *data = NULL;
    }

    // free the pending message handle
    dbus_pending_call_unref(pending);

    // free reply
    dbus_message_unref(msg);
    //dbus_connection_close(conn);

    return 0;
}

void dbus_release_message(char *data)
{
    if( data != NULL ) {
        free(data);
    }
}

/**
 * dbus发送signal
 * conn : dbus句柄
 * signal ： 信号
 * message ： 消息
 */
int dbus_send_signal(DBusConnection *conn,  const char *signal, const char* message)
{
    DBusMessage* msg;
    DBusMessageIter args;
    DBusError err;
    int ret;
    dbus_uint32_t serial = 0;
	char *object_str[128];
	char *interface_str[128];

	snprintf(object_str, 128, DBUS_OBJECT, signal);
	snprintf(interface_str, 128, DBUS_INTERFACE, signal);

    printf("(dbus)%s, object_str = %s, interface_str = %s, signal = %s, message = %s\n", __FUNCTION__, object_str, interface_str, signal, message);

    // create a signal & check for errors
    msg = dbus_message_new_signal(object_str, // object name of the signal
								interface_str, // interface name of the signal
								signal); // name of the signal

    if (NULL == msg)
    {
        printf("(dbus)Message Null\n");
        return -1;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &message)) {
        printf("(dbus)Out Of Memory!\n");
        return -1;
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) {
        printf("(dbus)Out Of Memory!\n");
        return -1;
    }

    dbus_connection_flush(conn);
    printf("(dbus)Signal Sent\n");

    // free the message and close the connection
    dbus_message_unref(msg);
//    dbus_connection_unref(conn);

    return 0;
}

/**
 * dbus接收监听，放到一个单独的线程中
 */
int dbus_recv_loop(DBusConnection *conn)
{
    printf("(dbus)%s", __FUNCTION__);
    while(1)
    {
        struct pollfd fds[MAX_WATCHES];
        DBusWatch *watch[MAX_WATCHES];
        int nfds, i;

        for (nfds = 0, i = 0; i <= max_i; ++i) {
            if (pollfds[i].fd == 0 ||
                !dbus_watch_get_enabled(watches[i]))
                continue;
            fds[nfds].fd = pollfds[i].fd;
            fds[nfds].events = pollfds[i].events;
            fds[nfds].revents = 0;
            watch[nfds] = watches[i];
            ++nfds;
        }
        if (poll(fds, nfds, -1) <= 0) {
            perror("poll");
            printf("(dbus)poll error");
            break;
        }
        for (i = 0; i < nfds; i++) {
            if (fds[i].revents) {
                fd_handler(conn, fds[i].revents, watch[i]);
            }
        }
    }
}
