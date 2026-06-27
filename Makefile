# Rogue Makefile

CC := gcc

SRC := \
	src/main.c \
	src/core/helper.c \
	src/core/colors.c \
	src/core/global_states.c \
	src/audio/sound.c \
	src/render/font.c \
	src/render/camera.c \
	src/gameplay/collision.c \
	src/gameplay/asset.c \
	src/gameplay/character/class_base_stats.c \
	src/gameplay/character/class_stat_growth.c \
	src/gameplay/personality/personality_results.c \
	src/gameplay/personality/personality_scenario.c \
	src/gameplay/personality/personality_test.c \
	src/ui/name_entry.c \
	src/platform/sdl_platform.c

OBJ := $(SRC:src/%.c=build/obj/%.o)
DEP := $(OBJ:.o=.d)

CPPFLAGS := \
    -Isrc \
    -Isrc/core \
    -Isrc/platform \
    -Isrc/render \
    -Isrc/gameplay \
    -Isrc/gameplay/character \
    -Isrc/gameplay/personality \
    -Isrc/ui \
    -Isrc/audio \
    -Ivendor/include
CFLAGS   := -g -Wall -Wextra -std=c99 -MMD -MP
LDLIBS   := -lm

ifeq ($(OS),Windows_NT)
	TARGET := build/rogue.exe
	CPPFLAGS += -Ivendor/include
	LDFLAGS  += -Lvendor/lib
	LDLIBS   := -lmingw32 -lSDL2main -lSDL2 -lm
else
	TARGET := build/rogue
	CPPFLAGS += $(shell sdl2-config --cflags)
	LDLIBS   += $(shell sdl2-config --libs)
endif

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(OBJ) -o $@ $(LDFLAGS) $(LDLIBS)
ifeq ($(OS),Windows_NT)
	cp -f bin/SDL2.dll build/SDL2.dll
endif

build/obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build

-include $(DEP)

.PHONY: all run clean