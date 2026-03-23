# Default to debug build
BUILD_TYPE ?= debug

# Chose the compiler you prefer
CC ?= clang

# Common flags for both builds
COMMON_CFLAGS := -std=c99 \
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
	-Wno-aggregate-return

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    COMMON_CFLAGS += -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk
    SOKOL_BACKEND := SOKOL_METAL
    SOKOL_CC      := $(CC) -x objective-c
    SOKOL_LDFLAGS := -framework Cocoa -framework Metal -framework MetalKit \
                     -framework QuartzCore -framework AudioToolbox
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

lib/sokol.o: CFLAGS := -D$(SOKOL_BACKEND) -O2
lib/sokol.o: lib/sokol.m
	$(SOKOL_CC) $(CFLAGS) -c $< -o $@

src/main: LDFLAGS := $(SOKOL_LDFLAGS)
src/main: CFLAGS += -D$(SOKOL_BACKEND)
src/main: lib/sokol.o

.PHONY: clean
clean:
	rm -rf src/**/*.dSYM **/*.o transcribling

.PHONY: start
start: src/main
	src/main
