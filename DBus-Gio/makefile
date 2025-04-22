CC = gcc

TARGET1 = dbus_send
TARGET2 = dbus_recv

SRCS1 = dbus_send.c
OBJS1 = $(patsubst %c, %o, $(SRCS1))

SRCS2 = dbus_recv.c
OBJS2 = $(patsubst %c, %o, $(SRCS2))

INC = -I/usr/include/dbus-1.0 -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include

LIB = -L/usr/lib/x86_64-linux-gnu -ldbus-1 -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lgmodule-2.0 -lgthread-2.0 -pthread -lm

.PHONY:all clean

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJS1)
	$(CC) -o $@ $^ $(LIB)

$(TARGET2): $(OBJS2)
	$(CC) -o $@ $^ $(LIB)

%.o:%.c
	$(CC) -c $(INC) $(LIB) $^

clean:
	rm -f $(OBJS) $(TARGET1) $(TARGET2)
