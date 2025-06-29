@echo off
setlocal enabledelayedexpansion

:: ========================================
:: ULTRATHINK BUILD SCRIPT FOR SELACO/ZDOOM
:: ========================================

echo.
echo ===================================================
echo   SELACO BUILD SCRIPT - ULTRATHINK MODE ACTIVATED
echo ===================================================
echo.

:: Set build configuration (Release, Debug, RelWithDebInfo, MinSizeRel)
set BUILD_TYPE=Release

:: Set build directory
set BUILD_DIR=build

:: Set number of parallel jobs for compilation (adjust based on your CPU)
set PARALLEL_JOBS=%NUMBER_OF_PROCESSORS%

:: Check if CMake is available
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] CMake not found in PATH!
    echo Please install CMake from https://cmake.org/download/
    echo.
    pause
    exit /b 1
)

:: Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" (
    echo [*] Creating build directory: %BUILD_DIR%
    mkdir "%BUILD_DIR%"
)

:: Change to build directory
cd "%BUILD_DIR%"

:: Configure with CMake
echo [*] Configuring project with CMake...
echo     Build Type: %BUILD_TYPE%
echo     mbedTLS: FORCE_INTERNAL_MBEDTLS=ON
echo     CMake Flags: -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -Wno-dev
echo.

cmake .. -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DFORCE_INTERNAL_MBEDTLS=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -Wno-dev

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo [*] Configuration successful!
echo [*] mbedTLS will be built for SSL/TLS support
echo.

:: Build the project
echo [*] Building project with %PARALLEL_JOBS% parallel jobs...
echo.

cmake --build . --config %BUILD_TYPE% --parallel %PARALLEL_JOBS%

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Build failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo ===================================================
echo   BUILD COMPLETED SUCCESSFULLY! 
echo ===================================================
echo.
echo Executable should be in: %CD%\%BUILD_TYPE%\ (or %CD%\ depending on generator)
echo.

:: Return to original directory
cd ..

:: Ask if user wants to run the executable
set /p RUN_EXEC="Do you want to run zdoom.exe now? (y/n): "
if /i "%RUN_EXEC%"=="y" (
    if exist "%BUILD_DIR%\%BUILD_TYPE%\zdoom.exe" (
        echo [*] Starting zdoom.exe...
        start "" "%BUILD_DIR%\%BUILD_TYPE%\zdoom.exe"
    ) else if exist "%BUILD_DIR%\zdoom.exe" (
        echo [*] Starting zdoom.exe...
        start "" "%BUILD_DIR%\zdoom.exe"
    ) else (
        echo [WARNING] Could not find zdoom.exe in expected locations
    )
)

pause