SHELL := cmd.exe

# ==================== CONFIGURATION ====================
SDL_INC_PATH = C:/SDL2/include
SDL_LIB_PATH = C:/SDL2/lib

SRC_DIR         = ..
SRC_DIR_CPU     = $(SRC_DIR)/cpu
SRC_DIR_SYSTEM  = $(SRC_DIR)/system
SRC_DIR_VC      = $(SRC_DIR)/videocards
SRC_DIR_EMU     = $(SRC_DIR)/emulator
BUILD_DIR       = ..\..\build
DIST_DIR        = ..\..\dist
DLL_DIR         = ..\..\dll

CC      = gcc
CXX     = g++
CFLAGS  = -Wall -Wextra -std=c11 -I$(SRC_DIR)/include -I"$(SDL_INC_PATH)"
CXXFLAGS= -Wall -Wextra -std=c++17 -I$(SRC_DIR)/include -I"$(SDL_INC_PATH)"
LDFLAGS = -L"$(SDL_LIB_PATH)" -lmingw32 -lSDL2main -lSDL2 -lcomdlg32

TARGET_NAME = emulator.exe
TARGET      = $(DIST_DIR)\$(TARGET_NAME)

# C sources: system, videocards, CPU backends
C_SOURCES   = $(wildcard $(SRC_DIR_SYSTEM)/*.c)
C_SOURCES  += $(wildcard $(SRC_DIR_VC)/*.c)
C_SOURCES  += $(wildcard $(SRC_DIR_CPU)/*/*.c)

# C++ sources: emulator framework and ImGui
GUI_CPP_SOURCES = $(SRC_DIR_EMU)/gui_main.cpp $(SRC_DIR_EMU)/gui_emulator.cpp $(wildcard $(SRC_DIR_EMU)/imgui*.cpp)
CONSOLE_CPP_SOURCES = $(SRC_DIR_EMU)/console_main.cpp

C_OBJECTS       = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))
GUI_OBJECTS     = $(patsubst $(SRC_DIR_EMU)/%.cpp, $(BUILD_DIR)/%.o, $(GUI_CPP_SOURCES))
CONSOLE_OBJECTS = $(patsubst $(SRC_DIR_EMU)/%.cpp, $(BUILD_DIR)/console_%.o, $(notdir $(CONSOLE_CPP_SOURCES)))

SDL_DLL_SRC = $(DLL_DIR)\SDL2.dll
SDL_DLL_DST = $(DIST_DIR)\SDL2.dll

.PHONY: all clean run console

all: $(DIST_DIR) $(TARGET) deploy_dlls
	@echo [SUCCESS] Build complete. Executable is in emulator/dist/

console: $(DIST_DIR) $(BUILD_DIR) $(C_OBJECTS) $(BUILD_DIR)/console_main.o
	@echo [LINK] Console emulator...
	$(CXX) $(C_OBJECTS) $(BUILD_DIR)/console_main.o -o $(DIST_DIR)/console_emu.exe $(LDFLAGS)

$(BUILD_DIR):
	@if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"

$(DIST_DIR):
	@if not exist "$(DIST_DIR)" mkdir "$(DIST_DIR)"

$(TARGET): $(C_OBJECTS) $(GUI_OBJECTS) | $(DIST_DIR)
	@echo [LINK] Linking $(TARGET_NAME)...
	$(CXX) $(C_OBJECTS) $(GUI_OBJECTS) -o $@ $(LDFLAGS)

# Pattern rules for C sources (preserve subdirectory structure)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir "$(subst /,\,$(dir $@))" >nul 2>&1 || (exit /b 0)
	$(CC) $(CFLAGS) -c $< -o $@

# Pattern rules for C++ emulator sources
$(BUILD_DIR)/%.o: $(SRC_DIR_EMU)/%.cpp | $(BUILD_DIR)
	@mkdir "$(subst /,\,$(dir $@))" >nul 2>&1 || (exit /b 0)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Console main gets a distinct object name to avoid colliding with gui_main
$(BUILD_DIR)/console_main.o: $(SRC_DIR_EMU)/console_main.cpp | $(BUILD_DIR)
	@mkdir "$(subst /,\,$(dir $@))" >nul 2>&1 || (exit /b 0)
	$(CXX) $(CXXFLAGS) -c $< -o $@

deploy_dlls: $(DIST_DIR)
	@echo [DEPLOY] Copying DLLs...
	@if exist "$(subst /,\,$(SDL_DLL_SRC))" (copy /Y "$(subst /,\,$(SDL_DLL_SRC))" "$(DIST_DIR)\" >nul) else (echo [WARNING] SDL2.dll not found in $(DLL_DIR)!)

clean:
	@echo [CLEAN] Removing build files...
	@if exist "$(BUILD_DIR)" rmdir /s /q "$(BUILD_DIR)"
	@if exist "$(DIST_DIR)" rmdir /s /q "$(DIST_DIR)"
