
# Copyright 2025 NXP

# SPDX-License-Identifier: BSD-3-Clause

# Compiler settings
CC ?= gcc
CXX ?= g++
 
# Compiler flags
CFLAGS = -Wall -g $(shell pkg-config --cflags wayland-client)
CXXFLAGS = -Wall -g $(shell pkg-config --cflags wayland-client)
LDFLAGS = $(shell pkg-config --libs wayland-client)
LIBS = -lopencv_core -lopencv_imgcodecs -lopencv_imgproc

# Build deps
WAYLAND_PROTOCOLS_DIR = $(shell pkg-config wayland-protocols --variable=pkgdatadir)

WAYLAND_SCANNER ?= wayland-scanner
# For cross-compilation, prefer host wayland-scanner if available
WAYLAND_SCANNER_HOST ?= $(shell which wayland-scanner 2>/dev/null)
ifneq ($(WAYLAND_SCANNER_HOST),)
	WAYLAND_SCANNER := $(WAYLAND_SCANNER_HOST)
endif

XDG_SHELL_PROTOCOL = $(WAYLAND_PROTOCOLS_DIR)/stable/xdg-shell/xdg-shell.xml
OUTPUT_HEADER = xdg-shell-client-protocol.h
OUTPUT_CODE = xdg-shell-client-protocol.c
 
# Target executable name
TARGET = imx-camera-rotation-opencv
 
# Source files
C_SOURCES = $(filter-out $(OUTPUT_CODE), $(wildcard *.c)) $(OUTPUT_CODE)
CPP_SOURCES = $(wildcard *.cpp)
 
# Object files
C_OBJECTS = $(C_SOURCES:.c=.o)
CPP_OBJECTS = $(CPP_SOURCES:.cpp=.o)
OBJECTS = $(C_OBJECTS) $(CPP_OBJECTS)
 
# Dependency files for automatic dependency tracking
DEPS = $(OBJECTS:.o=.d)
 
# Default target
all: $(TARGET)
 
# Link object files into the final executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS) $(LIBS)
 
# Compile C source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -MD -MP
 
# Compile C++ source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MP

# Explicit rule for xdg-shell-client-protocol.o
$(OUTPUT_CODE:.c=.o): $(OUTPUT_CODE) $(OUTPUT_HEADER)
	$(CC) $(CFLAGS) -c $(OUTPUT_CODE) -o $@ -MD -MP
 
# Include dependency files
-include $(DEPS)
 
# Generate Wayland protocol code
$(OUTPUT_HEADER):
	$(WAYLAND_SCANNER) client-header $(XDG_SHELL_PROTOCOL) $(OUTPUT_HEADER)

$(OUTPUT_CODE):
	$(WAYLAND_SCANNER) private-code $(XDG_SHELL_PROTOCOL) $(OUTPUT_CODE)
 
# Clean up generated files
clean:
	rm -f $(OBJECTS) $(DEPS) $(TARGET) $(OUTPUT_HEADER) $(OUTPUT_CODE)
 
# Phony targets
.PHONY: all clean
