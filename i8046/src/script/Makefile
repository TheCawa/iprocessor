SHELL := cmd.exe

# ==================== CONFIGURATION ====================
# Пути к SDL2
SDL_INC_PATH = C:/SDL2/include
SDL_LIB_PATH = C:/SDL2/lib

# Пути к проекту
SRC_DIR_CPU     = ../cpu
SRC_DIR_EMU     = ../emulator
BUILD_DIR       = ..\build
DIST_DIR        = ..\dist
DLL_DIR         = ..\dll

# Компиляторы и флаги
CC      = gcc
CXX     = g++
CFLAGS  = -Wall -Wextra -std=c11 -I../include -I"$(SDL_INC_PATH)"
CXXFLAGS= -Wall -Wextra -std=c++17 -I../include -I"$(SDL_INC_PATH)"
LDFLAGS = -L"$(SDL_LIB_PATH)" -lmingw32 -lSDL2main -lSDL2

# Исполняемый файл
TARGET_NAME = emulator.exe
TARGET      = $(DIST_DIR)\$(TARGET_NAME)

# ==================== SOURCES & OBJECTS ====================
# Auto-discovery
C_SOURCES   = $(wildcard $(SRC_DIR_CPU)/*.c)
CPP_SOURCES = $(wildcard $(SRC_DIR_EMU)/*.cpp)

# Object files mapping
C_OBJECTS   = $(patsubst $(SRC_DIR_CPU)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))
CPP_OBJECTS = $(patsubst $(SRC_DIR_EMU)/%.cpp, $(BUILD_DIR)/%.o, $(CPP_SOURCES))

ALL_OBJECTS = $(C_OBJECTS) $(CPP_OBJECTS)

# DLLs to copy
SDL_DLL_SRC = $(DLL_DIR)\SDL2.dll
SDL_DLL_DST = $(DIST_DIR)\SDL2.dll

# ==================== TARGETS ====================
.PHONY: all clean run

all: $(DIST_DIR) $(TARGET) deploy_dlls
	@echo [SUCCESS] Build complete. Executable is in src/dist/

# Create directories
$(BUILD_DIR):
	@if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"

$(DIST_DIR):
	@if not exist "$(DIST_DIR)" mkdir "$(DIST_DIR)"

# Linking
$(TARGET): $(ALL_OBJECTS) | $(DIST_DIR)
	@echo [LINK] Linking $(TARGET_NAME)...
	$(CXX) $(ALL_OBJECTS) -o $@ $(LDFLAGS)

# Compile C files
$(BUILD_DIR)/%.o: $(SRC_DIR_CPU)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C++ files
$(BUILD_DIR)/%.o: $(SRC_DIR_EMU)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Deploy DLLs
deploy_dlls: $(DIST_DIR)
	@echo [DEPLOY] Copying DLLs...
	@if exist "$(subst /,\,$(SDL_DLL_SRC))" (copy /Y "$(subst /,\,$(SDL_DLL_SRC))" "$(DIST_DIR)\" >nul) else (echo [WARNING] SDL2.dll not found in $(DLL_DIR)!)

# Run target
run: $(TARGET)
	@cd $(DIST_DIR) && .\$(TARGET_NAME)

# Clean
clean:
	@echo [CLEAN] Removing build files...
	@if exist "$(BUILD_DIR)" rmdir /s /q "$(BUILD_DIR)"
	@if exist "$(DIST_DIR)" rmdir /s /q "$(DIST_DIR)"