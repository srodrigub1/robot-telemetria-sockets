CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -O2
CFLAGS += -pthread
LDFLAGS ?= -pthread

SRCS = server.c world.c net.c protocol.c auth.c log.c telemetry.c
OBJS = $(SRCS:.c=.o)

TARGET = server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
