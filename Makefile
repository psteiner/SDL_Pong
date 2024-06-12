
.PHONY: all clean dirs

PROJECT_NAME            ?= pong
BUILD_MODE              ?= DEBUG
CC = gcc
MAKE ?= mingw32-make
SHELL=cmd
SDL_PATH=D:/SDL2/mingw
W64DEVKIT_PATH=D:/w64devkit/bin

# Define compiler flags: CFLAGS
#-------------------------------------------------------------------------------
#  -std=c99             defines C language mode (standard C from 1999 revision)
#  -Wall                turns on most, but not all, compiler warnings
CFLAGS = -std=c99 -Wall

ifeq ($(BUILD_MODE),DEBUG)
  # disables optimization, maximizes debug information, enables run-time
  # instrumentation, and provides linting
  CFLAGS += -ggdb3 -Wextra -Wdouble-promotion \
	-Wmissing-prototypes -Wstrict-prototypes \
	-fsanitize=undefined -fsanitize-undefined-trap-on-error
endif

# Define include paths for required headers: INC_PATH
#-------------------------------------------------------------------------------
INC_PATH = -I. -I$(SDL_PATH)/include/SDL2

# Define library paths containing required libs: LDFLAGS
#-------------------------------------------------------------------------------
LDFLAGS = -L. -L$(SDL_PATH)/lib

ifeq ($(BUILD_MODE), RELEASE)
# -s Remove all symbol table and relocation information from the executable
# -Wl,--subsystem,windows hides the console window
		LDFLAGS += -s -Wl,--subsystem,windows
endif

# Define libraries required on linking: LDLIBS
LDLIBS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer

# Define source code object files required
# see https://codereview.stackexchange.com/questions/74136/makefile-that-places-object-files-into-an-alternate-directory-bin
#-------------------------------------------------------------------------------
SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = $(BIN_DIR)/obj

# Define all object files from source files
SRCS = $(SRC_DIR)/$(PROJECT_NAME).c
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
EXE = $(BIN_DIR)/$(PROJECT_NAME).exe


all: dirs $(EXE)

# Project target defined by PROJECT_NAME
$(EXE): $(OBJS) 
	@echo +++ SRCS: $(SRCS)
	@echo +++ OBJS: $(OBJS)
	$(CC) $(CFLAGS) $(INC_PATH) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)

# Compile source files
# $< Name of first prerequisite
# $@ File name of the rule target
$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	@echo +++ input: $< output: $@
	$(CC) $(CFLAGS) $(INC_PATH) -c $< -o $@

# make bin/obj dirs
dirs:
	$(W64DEVKIT_PATH)/mkdir -p $(OBJ_DIR)

# Clean everything
clean: 
	@if exist $(BIN_DIR) (rmdir /s /q $(BIN_DIR)) else (echo no cleanup needed)
	@echo Cleaning done
