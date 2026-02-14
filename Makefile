#    ___________      __________ ____     ____              __ 
#   / ____/ ___/     |__  /__  // __ \   / __ \____  ____  / /_
#  / /    \__ \______ /_ < /_ </ / / /  / /_/ / __ \/ __ \/ __/
# / /___ ___/ /_____/__/ /__/ / /_/ /  / _, _/ /_/ / /_/ / /_  
# \____//____/     /____/____/\____/  /_/ |_|\____/\____/\__/  
#                                                              
#     __  ___      __        _____ __   
#    /  |/  /___ _/ /_____  / __(_) /__ 
#   / /|_/ / __ `/ //_/ _ \/ /_/ / / _ \
#  / /  / / /_/ / ,< /  __/ __/ / /  __/
# /_/  /_/\__,_/_/|_|\___/_/ /_/_/\___/ 
#                                       
# Maintainer: Kyle Gortych
# Date: 2026-01-25

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
# Compiler setup
# -------------------------
CC ?= gcc
CXX ?= $(if $(filter clang gcc,$(CC)),$(CC)++,g++)
export CC CXX

# -------------------------
# List of project directories
# -------------------------
PROJECTS := \
	1-2_OpenGLSample \
	2-2_Assignment \
	3-2_Assignment \
	4-2_Assignment \
	5-2_Assignment \
	6-2_Assignment \
	7-1_FinalProjectMilestones \
	8-2_Assignment

# Default target
.DEFAULT_GOAL := help

.PHONY: all clean help list $(PROJECTS)

# -------------------------
# Build all projects
# -------------------------
all: $(PROJECTS)
	@echo "All projects built with $(CXX)."

# -------------------------
# Build individual project
# -------------------------
$(PROJECTS):
	@if [ ! -d Projects/$@ ]; then \
		echo "Project folder Projects/$@ does not exist"; \
		exit 1; \
	fi
	@echo "Building project: $@ with $(CXX)"
	$(MAKE) $(JOBS) -C Projects/$@ CC=$(CC) CXX=$(CXX)

# -------------------------
# Clean all projects
# -------------------------
clean:
	@echo "Cleaning all projects..."
	@for p in $(PROJECTS); do \
		$(MAKE) $(JOBS) -C Projects/$$p clean; \
	done
	@echo "Clean complete."

# -------------------------
# List available projects
# -------------------------
list:
	@echo "Available projects:"
	@for p in $(PROJECTS); do \
		echo "  - $$p"; \
	done

# -------------------------
# Help / usage
# -------------------------
help:
	@echo ""
	@echo "CS-330 Root Makefile"
	@echo "===================="
	@echo ""
	@echo "Usage:"
	@echo "  make                 Show this help"
	@echo "  make all             Build all projects"
	@echo "  make <project>       Build a specific project"
	@echo "  make clean           Clean all projects"
	@echo "  make list            List available projects"
	@echo "  make CC=clang all    Use Clang instead of GCC"
	@echo ""
	@echo "Examples:"
	@echo "  make 2-2_Assignment CC=clang"
	@echo "  make 7-1_FinalProjectMilestones"
	@echo ""
	@echo "Notes:"
	@echo "  - Run 'nix-shell' before 'make' on NixOS"
	@echo "  - Windows users should use MSYS2, MinGW, or WSL"
	@echo ""
