#    ___________      __________ ____ 
#   / ____/ ___/     |__  /__  // __ \
#  / /    \__ \______ /_ < /_ </ / / /
# / /___ ___/ /_____/__/ /__/ / /_/ / 
# \____//____/     /____/____/\____/  
#     __  ___          __      __        ____ 
#    /  |/  /___  ____/ /_  __/ /__     ( __ )
#   / /|_/ / __ \/ __  / / / / / _ \   / __  |
#  / /  / / /_/ / /_/ / /_/ / /  __/  / /_/ / 
# /_/  /_/\____/\__,_/\__,_/_/\___/   \____/  
# 
# 2D Breakout Animation
#
# This project uses only GLFW and legacy OpenGL.
# No GLEW, glad, glm, or custom shaders needed.
#
#  Maintainer: Kyle Gortych
#  Date: 3/1/2026

# Compiler setup
CXX      ?= g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2

# Directories
SRCDIR   := Source
BUILDDIR := build
BINDIR   := build

# Source files and objects
SRCS := $(SRCDIR)/MainCode.cpp \
        $(SRCDIR)/Brick.cpp \
        $(SRCDIR)/Circle.cpp \
        $(SRCDIR)/Paddle.cpp \
        $(SRCDIR)/Renderer.cpp \
        $(SRCDIR)/GameState.cpp

OBJS := $(BUILDDIR)/MainCode.o \
        $(BUILDDIR)/Brick.o \
        $(BUILDDIR)/Circle.o \
        $(BUILDDIR)/Paddle.o \
        $(BUILDDIR)/Renderer.o \
        $(BUILDDIR)/GameState.o

# Output binary
TARGET := $(BINDIR)/8-2_Assignment

# Platform detection and library flags
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)

ifeq ($(UNAME_S),Darwin)
    # macOS — use framework OpenGL
    LDFLAGS  := -lglfw -framework OpenGL -lm
else ifeq ($(UNAME_S),Linux)
    # Linux / NixOS
    LDFLAGS  := -lglfw -lGL -lm
else
    # Windows (MSYS2 / MinGW)
    LDFLAGS  := -lglfw3 -lopengl32 -lgdi32 -lm
endif

# pkg-config override for NixOS 
PKG_CONFIG := $(shell command -v pkg-config 2>/dev/null)
ifdef PKG_CONFIG
    GLFW_CFLAGS  := $(shell pkg-config --cflags glfw3 2>/dev/null)
    GLFW_LDFLAGS := $(shell pkg-config --libs   glfw3 2>/dev/null)
    ifneq ($(GLFW_LDFLAGS),)
        CXXFLAGS += $(GLFW_CFLAGS)
        LDFLAGS   = $(GLFW_LDFLAGS) -lGL -lm
    endif
endif

# Targets
.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS) | $(BINDIR)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)
	@echo ""
	@echo "Build complete: $(TARGET)"
	@echo "Run with:  make run   or   ./$(TARGET)"

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) -c $< -o $@

$(BUILDDIR) $(BINDIR):
	mkdir -p $@

clean:
	rm -rf $(BUILDDIR)
	@echo "Clean complete."

run: $(TARGET)
	./$(TARGET)
