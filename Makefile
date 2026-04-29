# Force using cmd.exe on Windows
SHELL = cmd.exe

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -MMD -MP
LDFLAGS = -lmingw32 -lSDL2main -lSDL2 -lm

# Directories
SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = $(BIN_DIR)/obj
INC_DIR = include
LIB_DIR = lib
VENDOR_SDL_DIR = vendor/SDL2/include
VENDOR_DIR = vendor

# Include paths
INCLUDES = -I$(INC_DIR) -I$(VENDOR_SDL_DIR) -I$(VENDOR_DIR)

# Library paths
LIB_PATHS = -L$(LIB_DIR)

# Source and object files (including subdirectories)
SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/**/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Vendor sources (upng)
VENDOR_SRCS = vendor/upng/upng.c
VENDOR_OBJS = $(OBJ_DIR)/upng.o

# Dependency files (auto-generated from headers)
DEPS = $(OBJS:.o=.d)

# Output executable
TARGET = $(BIN_DIR)/renderer.exe

# Object subdirectories
OBJ_SUBDIRS = $(OBJ_DIR)/Math $(OBJ_DIR)/Mesh $(OBJ_DIR)/Lighting $(OBJ_DIR)/Rendering $(OBJ_DIR)/Graphics

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS) $(VENDOR_OBJS)
	$(CC) $(OBJS) $(VENDOR_OBJS) -o $@ $(LIB_PATHS) $(LDFLAGS)

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) $(OBJ_SUBDIRS)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile vendor upng source
$(OBJ_DIR)/upng.o: vendor/upng/upng.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Create obj directory if it doesn't exist
$(OBJ_DIR):
	@if not exist "$(subst /,\,$(OBJ_DIR))" mkdir "$(subst /,\,$(OBJ_DIR))"

# Create obj subdirectories
$(OBJ_SUBDIRS):
	@if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)"

# Include auto-generated dependency files
-include $(DEPS)

# Clean build artifacts
clean:
	@if exist "$(subst /,\,$(OBJ_DIR))\*.o" del /Q "$(subst /,\,$(OBJ_DIR))\*.o"
	@if exist "$(subst /,\,$(OBJ_DIR))\*.d" del /Q "$(subst /,\,$(OBJ_DIR))\*.d"
	@if exist "$(subst /,\,$(TARGET))" del /Q "$(subst /,\,$(TARGET))"

# Rebuild everything
rebuild: clean all

# Run the program
run: $(TARGET)
	$(TARGET)

.PHONY: all clean rebuild run
