CC = gcc
CFLAGS = -Wall -g -pthread -I.
TARGET = chat_server
SOURCES = main.c network.c thread_pool.c client_list.c message_handler.c user.c cJSON.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean