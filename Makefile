CC = gcc

INCLUDE_DIRS := $(shell find include/ -type d)

CFLAGS := -Wall -o -g $(addprefix -I,$(INCLUDE_DIRS)) `pkg-config --cflags glib-2.0`
LDFLAGS := `pkg-config --libs glib-2.0`

SRC_SERVER := $(shell find src/server/ -maxdepth 1 -type f)
SRC_CLIENT := $(shell find src/client/ -maxdepth 1 -type f)
SRC		   := $(shell find src -maxdepth 1 -type f)

OBJ_SERVER := $(patsubst src/server/%.c,obj/server/%.o,$(SRC_SERVER))
OBJ_CLIENT := $(patsubst src/client/%.c,obj/client/%.o,$(SRC_CLIENT))
OBJ		   := $(patsubst src/%.c,obj/%.o,$(SRC))

all: folders dserver dclient

dserver: bin/dserver
dclient: bin/dclient

folders: 
	@mkdir -p obj/client obj/server bin tmp

bin/dserver: $(OBJ_SERVER) $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

bin/dclient: $(OBJ_CLIENT) $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

obj/%.o: src/%.c | folders
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj bin tmp