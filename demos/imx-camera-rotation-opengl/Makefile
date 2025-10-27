# Compiler settings
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter -g
PKG_CONFIG ?= pkg-config

# Host deps
WAYLAND_FLAGS = $(shell $(PKG_CONFIG) wayland-client --cflags --libs)
WAYLAND_PROTOCOLS_DIR = $(shell $(PKG_CONFIG) wayland-protocols --variable=pkgdatadir)
LIBS = -lwayland-egl -lEGL -lGLESv2 -lm -lg2d

# Build deps
WAYLAND_SCANNER ?= wayland-scanner
# For cross-compilation, prefer host wayland-scanner if available
WAYLAND_SCANNER_HOST ?= $(shell which wayland-scanner 2>/dev/null)
ifneq ($(WAYLAND_SCANNER_HOST),)
	WAYLAND_SCANNER := $(WAYLAND_SCANNER_HOST)
endif

XDG_SHELL_PROTOCOL = $(WAYLAND_PROTOCOLS_DIR)/stable/xdg-shell/xdg-shell.xml
OUTPUT_HEADER = xdg-shell-client-protocol.h
OUTPUT_CODE = xdg-shell-client-protocol.c

HEADERS = $(OUTPUT_HEADER)
SOURCES = $(OUTPUT_CODE) main.c 

# Target executable name
TARGET = imx-camera-rotation-opengl

all: $(TARGET)

$(TARGET): $(HEADERS) $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(WAYLAND_FLAGS) $(LIBS)

$(OUTPUT_HEADER):
	$(WAYLAND_SCANNER) client-header $(XDG_SHELL_PROTOCOL) $(OUTPUT_HEADER)

$(OUTPUT_CODE):
	$(WAYLAND_SCANNER) private-code $(XDG_SHELL_PROTOCOL) $(OUTPUT_CODE)

.PHONY: clean
clean:
	$(RM) $(TARGET) $(OUTPUT_HEADER) $(OUTPUT_CODE)