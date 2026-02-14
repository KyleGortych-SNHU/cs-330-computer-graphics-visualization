#    ___________      __________ ____ 
#   / ____/ ___/     |__  /__  // __ \
#  / /    \__ \______ /_ < /_ </ / / /
# / /___ ___/ /_____/__/ /__/ / /_/ / 
# \____//____/     /____/____/\____/  
#                                     
#     ___              _                                  __     _____
#    /   |  __________(_)___ _____  ____ ___  ___  ____  / /_   |__  /
#   / /| | / ___/ ___/ / __ `/ __ \/ __ `__ \/ _ \/ __ \/ __/    /_ < 
#  / ___ |(__  |__  ) / /_/ / / / / / / / / /  __/ / / / /_    ___/ / 
#
#  Maintainer: Kyle Gortych
#  Date:

# -------------------------
# Auto-parallel setup (cross-platform)
# -------------------------
ifeq ($(OS),Windows_NT)
  JOBS :=
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Darwin)
    JOBS := -j$(shell sysctl -n hw.ncpu)
  else
    JOBS := -j$(shell nproc)
  endif
endif

# -------------------------
# Toolchain (inherited from root Makefile)
# -------------------------
CXX ?= g++
RM  := rm -f
MKDIR := mkdir -p

# -------------------------
# Output
# -------------------------
TARGET := Assignment8
BUILD_DIR := build

# -------------------------
# Directories
# -------------------------
SRC_DIR := Source
UTIL_DIR := ../../Utilities
SHAPE_DIR := ../../3DShapes

# -------------------------
# Source files
# -------------------------
SOURCES := \
	$(SRC_DIR)/MainCode.cpp \
	$(SRC_DIR)/SceneManager.cpp \
	$(SRC_DIR)/ViewManager.cpp \
	$(UTIL_DIR)/ShaderManager.cpp \
	$(SHAPE_DIR)/ShapeMeshes.cpp

OBJECTS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

# -------------------------
# Compiler flags
# -------------------------
DEFINES  := -DGLM_ENABLE_EXPERIMENTAL
INCLUDES := -I$(SRC_DIR) -I$(UTIL_DIR) -I$(SHAPE_DIR)
CXXFLAGS := -std=c++17 -Wall -Wextra $(DEFINES) $(INCLUDES)

# -------------------------
# Libraries (cross-platform)
# -------------------------
ifeq ($(OS),Windows_NT)
	LDLIBS := -lglfw3dll -lglew32 -lopengl32
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		LDLIBS := -lglew -lglfw -framework OpenGL
	else
		LDLIBS := -lGLEW -lglfw -lGL
	endif
endif

# -------------------------
# Targets
# -------------------------
.PHONY: all clean

all: $(TARGET)

# Link the final executable
$(TARGET): $(OBJECTS)
	@$(CXX) $(OBJECTS) -o $@ $(LDLIBS)

# Compile each source into build folder
$(BUILD_DIR)/%.o: %.cpp
	@$(MKDIR) $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean target
clean:
	@$(RM) $(TARGET) $(OBJECTS)
