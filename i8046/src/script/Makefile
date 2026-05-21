SHELL := cmd.exe

# Compiler settings
CC      = gcc
CXX     = g++
CFLAGS  = -Wall -Wextra -std=c11 -I../include
CXXFLAGS= -Wall -Wextra -std=c++17 -I../include
LDFLAGS = 

# Directories
SRC_DIR_CPU     = ../cpu
SRC_DIR_EMU     = ../emulator
BUILD_DIR       = ..\build
TARGET          = $(BUILD_DIR)\emulator.exe

# Auto-discovery of source files
C_SOURCES   = $(wildcard $(SRC_DIR_CPU)/*.c)
CPP_SOURCES = $(wildcard $(SRC_DIR_EMU)/*.cpp)

# Object files generation
C_OBJECTS   = $(patsubst $(SRC_DIR_CPU)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))
CPP_OBJECTS = $(patsubst $(SRC_DIR_EMU)/%.cpp, $(BUILD_DIR)/%.o, $(CPP_SOURCES))

ALL_OBJECTS = $(C_OBJECTS) $(CPP_OBJECTS)

# Default target
all: $(BUILD_DIR) $(TARGET)

# Create build directory if it doesn't exist
$(BUILD_DIR):
	@if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"

# Linking
$(TARGET): $(ALL_OBJECTS)
	$(CXX) $(ALL_OBJECTS) -o $@ $(LDFLAGS)

# Compile C files
$(BUILD_DIR)/%.o: $(SRC_DIR_CPU)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C++ files
$(BUILD_DIR)/%.o: $(SRC_DIR_EMU)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	@if exist "$(BUILD_DIR)" rmdir /s /q "$(BUILD_DIR)"

.PHONY: all clean