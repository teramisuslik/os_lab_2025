CC=gcc
CFLAGS=-I. -Wall
LDFLAGS=-lpthread

PORT1=20001
PORT2=20002
TNUM=4

K=5
MOD=100
SERVERS_FILE=servers.txt


all: client server

client: client.c libcommon.so
	$(CC) -o client client.c -L. -lcommon $(CFLAGS) $(LDFLAGS)

server: server.c libcommon.so
	$(CC) -o server server.c -L. -lcommon $(CFLAGS) $(LDFLAGS)


libcommon.so: common.o
	$(CC) -shared -o libcommon.so common.o

common.o: common.c common.h
	$(CC) -fPIC -c common.c -o common.o $(CFLAGS)

server1:
	LD_LIBRARY_PATH=. ./server --port $(PORT1) --tnum $(TNUM)

server2:
	LD_LIBRARY_PATH=. ./server --port $(PORT2) --tnum $(TNUM)

client-run:
	LD_LIBRARY_PATH=. ./client --k $(K) --mod $(MOD) --servers $(SERVERS_FILE)

stop:
	pkill server || true

setup:
	@echo "127.0.0.1:$(PORT1)" > $(SERVERS_FILE)
	@echo "127.0.0.1:$(PORT2)" >> $(SERVERS_FILE)
	@echo "Created $(SERVERS_FILE) with ports $(PORT1), $(PORT2)"

clean:
	rm -f client server $(SERVERS_FILE) libcommon.so common.o
