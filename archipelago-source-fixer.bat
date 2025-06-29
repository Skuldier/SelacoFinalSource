@echo off
:: Basic fix using only findstr and basic commands

echo.
echo ========================================
echo   BASIC ARCHIPELAGO FIX
echo ========================================
echo.

:: Backup files
echo [*] Creating backups...
copy "src\archipelago\archipelago_socket.h" "src\archipelago\archipelago_socket.h.backup" >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Could not backup archipelago_socket.h
    pause
    exit /b 1
)

:: Fix Printf in archipelago_socket.h
echo [*] Fixing Printf conflict...
echo [*] Creating fixed version of archipelago_socket.h...

:: Use findstr to exclude the Printf line
findstr /V "void Printf(const char" "src\archipelago\archipelago_socket.h" > "src\archipelago\archipelago_socket.h.new"

if exist "src\archipelago\archipelago_socket.h.new" (
    move /Y "src\archipelago\archipelago_socket.h.new" "src\archipelago\archipelago_socket.h" >nul
    echo [OK] Removed Printf declaration
) else (
    echo [ERROR] Failed to create fixed file
    pause
    exit /b 1
)

echo.
echo ========================================
echo   PARTIAL FIX COMPLETE
echo ========================================
echo.
echo The Printf error is now fixed!
echo.
echo To complete the fix, you need to manually:
echo   1. Open src\archipelago\archipelago_commands.cpp in Notepad
echo   2. Search for: archipelago_test_raw
echo   3. Delete that entire CCMD function
echo.
echo Then:
echo   rmdir /s /q build
echo   build_selaco.bat
echo.
set /p OPEN_NOW="Open archipelago_commands.cpp now? (y/n): "
if /i "%OPEN_NOW%"=="y" (
    notepad "src\archipelago\archipelago_commands.cpp"
)

echo.
pause