CC = gcc
CFLAGS = -Wall -g -Iinclude
#LDFLAGS =

all: folders dserver dclient

dserver: bin/dserver
dclient: bin/dclient

folders: 
	@mkdir -p obj/client obj/server bin tmp

bin/dserver: obj/server/main.o
	$(CC) $(LDFLAGS) $^ -o $@

bin/dclient: obj/client/main.o
	$(CC) $(LDFLAGS) $^ -o $@

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj bin tmp
