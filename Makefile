CC = gcc
CFLAGS = -Wall -o -g -Iinclude -Iinclude/server -Iinclude/client 
#LDFLAGS =

all: folders dserver dclient

dserver: bin/dserver
dclient: bin/dclient

folders: 
	@mkdir -p obj/client obj/server bin tmp

bin/dserver: obj/server/main.o obj/server/server.o obj/protocol.o obj/utils.o
	$(CC) $(LDFLAGS) $^ -o $@

bin/dclient: obj/client/main.o obj/client/client.o obj/protocol.o obj/utils.o
	$(CC) $(LDFLAGS) $^ -o $@

obj/server/%.o: src/server/%.c | folders
	$(CC) $(CFLAGS) -c $< -o $@

obj/client/%.o: src/client/%.c | folders
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.c | folders
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj bin tmp
