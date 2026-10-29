# ─────────────────────────────────────────────────────────────
#  RPG — build system
#  make                 -> native linux debug build  (build/game)
#  make run             -> build + launch it
#  make win             -> 64-bit windows .exe       (build/game.exe)
#  make RELEASE=1 win   -> optimised, no console window
#  make clean           -> nuke build/
#
#  if raylib is not in /usr/local (e.g. on the school laptop, installed
#  into $HOME/.local without root), pass PREFIX:
#      make PREFIX=$HOME/.local
# ─────────────────────────────────────────────────────────────

NAME    := game
SRC_DIR := src
INC_DIR := include
BIN_DIR := build

# unpack the raylib "win64_mingw-w64" release here, so you end up with
#   lib/raylib-win/include/raylib.h
#   lib/raylib-win/lib/libraylib.a
RAYLIB_WIN := lib/raylib-win

# where raylib lives on this machine. ?= means "use this unless
# something else (the command line, the environment) already set it"
PREFIX ?= /usr/local

SRCS    := $(wildcard $(SRC_DIR)/*.c)
OBJ_LIN := $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/linux/%.o,$(SRCS))
OBJ_WIN := $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/win/%.o,$(SRCS))

# -MMD -MP makes gcc emit .d files listing which headers each .o used,
# so editing a header actually triggers a rebuild. see the -include below.
CFLAGS := -std=c99 -Wall -Wextra -Wno-unused-parameter -I$(INC_DIR) -MMD -MP

ifdef RELEASE
	CFLAGS      += -O2 -DNDEBUG
	LDFLAGS_WIN += -mwindows -s
else
	CFLAGS      += -g -O0
endif

# raylib is built from source as a static lib.
# -L$(PREFIX)/lib: gcc does not always look there by default.
# -lX11 and friends: needed when linking against static raylib.
CC_LIN  := gcc
CF_LIN  := -I$(PREFIX)/include
LIB_LIN := -L$(PREFIX)/lib -lraylib -lm -ldl -lpthread \
           -lX11 -lXrandr -lXinerama -lXcursor -lXi

# make ASAN=1 -> build with AddressSanitizer (leaks, overflows, use-after-free)
ifdef ASAN
	CFLAGS  += -fsanitize=address -fno-omit-frame-pointer
	LIB_LIN += -fsanitize=address
endif

CC_WIN  := x86_64-w64-mingw32-gcc
CF_WIN  := -I$(RAYLIB_WIN)/include
LIB_WIN := -L$(RAYLIB_WIN)/lib -lraylib -lopengl32 -lgdi32 -lwinmm \
           -static -static-libgcc

# ─── rules ───────────────────────────────────────────────────

.PHONY: all win run clean

all: $(BIN_DIR)/$(NAME)

$(BIN_DIR)/$(NAME): $(OBJ_LIN)
	$(CC_LIN) $^ -o $@ $(LIB_LIN)

$(BIN_DIR)/linux/%.o: $(SRC_DIR)/%.c | $(BIN_DIR)/linux
	$(CC_LIN) $(CFLAGS) $(CF_LIN) -c $< -o $@

win: $(BIN_DIR)/$(NAME).exe

$(BIN_DIR)/$(NAME).exe: $(OBJ_WIN)
	$(CC_WIN) $^ -o $@ $(LIB_WIN) $(LDFLAGS_WIN)

$(BIN_DIR)/win/%.o: $(SRC_DIR)/%.c | $(BIN_DIR)/win
	$(CC_WIN) $(CFLAGS) $(CF_WIN) -c $< -o $@

$(BIN_DIR)/linux $(BIN_DIR)/win:
	@mkdir -p $@

run: all
	./$(BIN_DIR)/$(NAME)

clean:
	rm -rf $(BIN_DIR)

# pull in the auto-generated header dependency files, if they exist.
# the leading - means "don't cry if they don't"
-include $(OBJ_LIN:.o=.d) $(OBJ_WIN:.o=.d)