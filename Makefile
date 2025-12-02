CC = gcc
CFLAGS = -std=c17 -Werror
COMMON = common.o log.o

all: client server proxy

client: client.o $(COMMON)
	$(CC) $(CFLAGS) -o client client.o $(COMMON)

server: server.o $(COMMON)
	$(CC) $(CFLAGS) -o server server.o $(COMMON)

proxy: proxy.o $(COMMON)
	$(CC) $(CFLAGS) -o proxy proxy.o $(COMMON)

%.o: %.c common.h log.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f client server proxy *.o