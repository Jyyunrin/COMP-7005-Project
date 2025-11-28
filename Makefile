CC = gcc
CFLAGS = -std=c17 -Werror
COMMON = common.o

all: client server proxy

client: client.c $(COMMON)
	$(CC) $(CFLAGS) -o client client.c $(COMMON)

server: server.c $(COMMON)
	$(CC) $(CFLAGS) -o server server.c $(COMMON)

proxy: proxy.c $(COMMON)
	$(CC) $(CFLAGS) -o proxy proxy.c $(COMMON)

%.o: %.c common.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f client server proxy *.o