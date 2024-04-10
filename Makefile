.POSIX:
OBJS = src/pong.c
BUILD_DIR = build
CC = gcc
CFLAGS = -fdiagnostics-color=always -g -W -std=c17
LDFLAGS = -LD:/SDL2/lib
LDLIBS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image
INCLUDES = D:/SDL2/include/SDL2
OBJ_NAME = pong
EXE = build\\$(OBJ_NAME).exe

all: $(OBJS)
	@[ -d $(BUILD_DIR) ] || mkdir -p $(BUILD_DIR) 
	$(CC) $(OBJS) -I$(INCLUDES) $(LDLIBS) $(LDFLAGS) $(CFLAGS) -o $(EXE)


.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
