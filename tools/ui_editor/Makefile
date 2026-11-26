# ─────────────────────────────────────────────────────────────
#  ui_editor: canvas editor for the RPG's menus.
#  Lives outside the game: the game's Makefile only builds ../../src.
#
#  make            -> build/ui_editor
#  make run        -> build + open it on this repo
#  make test       -> parse every screen in the game, export, re-read, compare
#  make clean
#
#  raylib in /usr/local instead of ~/.local?  make PREFIX=/usr/local
# ─────────────────────────────────────────────────────────────

NAME    := ui_editor
BIN_DIR := build
PREFIX  ?= $(HOME)/.local
ROOT    := ../..

SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,$(BIN_DIR)/%.o,$(SRCS))

CFLAGS := -std=c99 -Wall -Wextra -Wno-unused-parameter -g -O1 -MMD -MP \
          -Isrc -Ivendor -I$(PREFIX)/include -D_DEFAULT_SOURCE
LIBS   := -L$(PREFIX)/lib -lraylib -lm -ldl -lpthread \
          -lX11 -lXrandr -lXinerama -lXcursor -lXi

ifdef ASAN
	CFLAGS += -fsanitize=address -fno-omit-frame-pointer
	LIBS   += -fsanitize=address
endif

.PHONY: all run test clean

all: $(BIN_DIR)/$(NAME)

$(BIN_DIR)/$(NAME): $(OBJS)
	$(CC) $^ -o $@ $(LIBS)

# raygui is vendored as-is: compile it without our warning flags
$(BIN_DIR)/raygui_impl.o: src/raygui_impl.c | $(BIN_DIR)
	$(CC) -std=c99 -O1 -w -Ivendor -I$(PREFIX)/include -c $< -o $@

$(BIN_DIR)/%.o: src/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	@mkdir -p $@

run: all
	./$(BIN_DIR)/$(NAME) $(ROOT)

test: all
	./$(BIN_DIR)/$(NAME) $(ROOT) --selftest

clean:
	rm -rf $(BIN_DIR)

-include $(OBJS:.o=.d)
