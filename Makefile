
.PHONY: all clean dirs

PROJECT_NAME            ?= pong
# Build mode for project: DEBUG or RELEASE
BUILD_MODE              ?= DEBUG
CC = gcc
MAKE ?= mingw32-make
SHELL=cmd

# Define compiler flags: CFLAGS
#-------------------------------------------------------------------------------
#  -g                   include debug information on compilation
#  -s                   strip unnecessary data from build
#  -Wall                turns on most, but not all, compiler warnings
#  -std=c99             defines C language mode (standard C from 1999 revision)
#  -std=gnu99           defines C language mode (GNU C from 1999 revision)
#  -Wno-missing-braces  ignore invalid warning (GCC bug 53119)
#  -Wno-unused-value    ignore unused return values of some functions (i.e. fread())
#  -D_DEFAULT_SOURCE    use with -std=c99 on Linux, required for timespec
CFLAGS = -std=c99 -Wall -Wno-missing-braces -Wunused-result

ifeq ($(BUILD_MODE),DEBUG)
  # disables optimization, maximizes debug information, enables run-time
  # instrumentation, and provides linting
  CFLAGS += -ggdb3 -Wextra -Wdouble-promotion \
	-Wmissing-prototypes -Wstrict-prototypes \
	-fsanitize=undefined -fsanitize-undefined-trap-on-error
endif

# Define include paths for required headers: INC_PATH
#-------------------------------------------------------------------------------
INC_PATH = -I. -ID:/SDL2/mingw/include/SDL2

# Define library paths containing required libs: LDFLAGS
#-------------------------------------------------------------------------------
LDFLAGS = -L. -LD:/SDL2/mingw/lib

# -Wl,--subsystem,windows hides the console window
ifeq ($(BUILD_MODE), RELEASE)
		LDFLAGS += -s -Wl,--subsystem,windows
endif

# Define libraries required on linking: LDLIBS
# NOTE: To link libraries (lib<name>.so or lib<name>.a), use -l<name>
#-------------------------------------------------------------------------------
# Libraries for Windows desktop compilation
# NOTE: WinMM library required to set high-res timer resolution
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

dirs:
	D:\w64devkit\bin\mkdir -p $(OBJ_DIR)

# Clean everything
clean: 
	@if exist $(BIN_DIR) (rmdir /s /q $(BIN_DIR)) else (echo no cleanup needed)
	@echo Cleaning done
