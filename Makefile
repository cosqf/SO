CC = gcc
CFLAGS = -Wall -g $(shell pkg-config --cflags glib-2.0) -Iinclude -Iinclude/server -Iinclude/client
LDFLAGS = $(shell pkg-config --libs glib-2.0)

all: folders dserver dclient

dserver: bin/dserver
dclient: bin/dclient

folders: 
	@mkdir -p obj/client obj/server bin tmp

bin/dserver: obj/server/main.o obj/server/server.o obj/protocol.o obj/utils.o obj/server/services.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

bin/dclient: obj/client/main.o obj/client/client.o obj/protocol.o obj/utils.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

obj/server/%.o: src/server/%.c | folders
	$(CC) $(CFLAGS) -c $< -o $@

obj/client/%.o: src/client/%.c | folders
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.c | folders
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj bin tmp
