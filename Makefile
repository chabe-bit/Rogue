CC := gcc
TARGET := build/rogue.exe

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

HEADERS := $(shell find src -name '*.h')

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

CFLAGS := \
	-g \
	-Wall \
	-Wextra \
	-std=c99

LIBS := \
	-Lvendor/lib \
	-lmingw32 \
	-lSDL2main \
	-lSDL2 \
	-lm

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	@mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)
	cp -f bin/SDL2.dll build/SDL2.dll

run: $(TARGET)
	cd build && ./rogue.exe

clean:
	rm -rf build

.PHONY: all run clean
