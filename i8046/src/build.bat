@echo off
echo [BUILD] Starting build process...
cd script
mingw32-make
if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    cd ..
    pause
    exit /b %errorlevel%
)
echo [SUCCESS] Build complete. Executable is in src/build/emulator.exe
cd ..
pause