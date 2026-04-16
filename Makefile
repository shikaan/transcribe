# Default to debug build
BUILD_TYPE ?= debug

# Chose the compiler you prefer
CC ?= clang

# Common flags for both builds
COMMON_CFLAGS := -std=c11 \
	-Wall \
	-Wextra \
	-Werror \
	-pedantic \
	-fdiagnostics-color=always \
	-fno-common \
	-Winit-self \
	-Wfloat-equal \
	-Wundef \
	-Wshadow \
	-Wpointer-arith \
	-Wcast-align \
	-Wstrict-prototypes \
	-Wstrict-overflow=5 \
	-Wwrite-strings \
	-Waggregate-return \
	-Wcast-qual \
	-Wswitch-default \
	-Wswitch-enum \
	-Wconversion \
	-Wno-ignored-qualifiers \
	-Wno-aggregate-return \
	-I vnd -I src -I vnd/rubberband

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    COMMON_CFLAGS += -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk
    SOKOL_BACKEND := SOKOL_METAL
    SOKOL_CC      := $(CC) -x objective-c
    SOKOL_LDFLAGS := -framework Cocoa -framework Metal -framework MetalKit \
                     -framework QuartzCore -framework AudioToolbox \
                     -framework Accelerate
else ifeq ($(UNAME_S),Linux)
    SOKOL_BACKEND := SOKOL_GLCORE
    SOKOL_CC      := $(CC)
    SOKOL_LDFLAGS := -lGL -lX11 -lXi -lXcursor -ldl -lm -lpthread
else
    SOKOL_BACKEND := SOKOL_D3D11
    SOKOL_CC      := $(CC)
    SOKOL_LDFLAGS := -ld3d11 -ldxgi -ld3dcompiler
endif

# Debug-specific flags
DEBUG_CFLAGS := -g -O0 -fsanitize=address,undefined -DDEBUG

# Release-specific flags
RELEASE_CFLAGS := -O2 -DNDEBUG

# Set flags based on build type
ifeq ($(BUILD_TYPE),release)
    CFLAGS := $(COMMON_CFLAGS) $(RELEASE_CFLAGS)
else
    CFLAGS := $(COMMON_CFLAGS) $(DEBUG_CFLAGS)
endif

###########

all: src/main

tools/fonts/packer: CFLAGS :=-O2

vnd/sokol.o: CFLAGS := -D$(SOKOL_BACKEND) -O2
vnd/sokol.o: vnd/sokol.m
	$(SOKOL_CC) $(CFLAGS) -c $< -o $@

vnd/miniaudio.o: CFLAGS :=-O2

vnd/tinyfiledialogs.o: CFLAGS := -O2

vnd/rubberband.o: vnd/rubberband/single/RubberBandSingle.cpp
	$(CXX) -O2 -c $< -o $@

src/gfx/shader.h: src/gfx/shader.glsl
	tools/sokol-tools/bin/osx_arm64/sokol-shdc \
	--input $< \
	--output $@ \
	-l metal_macos

FONT_OBJECTS := $(patsubst %.c,%.o,$(wildcard src/gfx/fonts/*@2x.c))
src/gfx/fonts/%.o: CFLAGS := -O2
src/gfx/gfx.o: CFLAGS += -D$(SOKOL_BACKEND)
src/gfx/gfx.o: src/gfx/gfx.c | src/gfx/shader.h

src/main: LDFLAGS := $(SOKOL_LDFLAGS) -lc++
src/main: CFLAGS += -D$(SOKOL_BACKEND)
src/main: vnd/sokol.o vnd/miniaudio.o vnd/tinyfiledialogs.o vnd/rubberband.o \
	src/gfx/gfx.o src/ui/ui.o src/audio/audio.o src/audio/samples.o \
	src/input/input.o src/prof/prof.o src/fmt/fmt.o $(FONT_OBJECTS) \
	| src/gfx/shader.h

.PHONY: assets
assets: tools/fonts/packer
	mkdir -p src/gfx/fonts
	rm -f src/gfx/fonts/*
	tools/fonts/packer assets/Icons.ttf 14
	tools/fonts/packer assets/Icons.ttf 20
	tools/fonts/packer assets/Icons.ttf 32
	tools/fonts/packer assets/InterDisplayRegular.ttf 12
	tools/fonts/packer assets/InterDisplayRegular.ttf 14
	tools/fonts/packer assets/InterDisplaySemiBold.ttf 14
	tools/fonts/packer assets/JetBrainsMonoRegular.ttf 12
	tools/fonts/packer assets/JetBrainsMonoRegular.ttf 14
	tools/fonts/packer assets/JetBrainsMonoRegular.ttf 20

.PHONY: clean
clean:
	rm -rf src/*.dSYM src/*.o src/**/*.dSYM src/**/*.o src/main

.PHONY: start
start: src/main
	src/main

.PHONY: config
config:
	compiledb make

# The extra arg is required because clang-tidy breaks (due to elevating the
# warning to error) before the rule could be disabled in .clang-tidy
.PHONY: lint
lint:
	run-clang-tidy -fix -format -quiet \
		-extra-arg=-Wno-unused-command-line-argument 'src/.*'
