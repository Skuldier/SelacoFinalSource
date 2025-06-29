@echo off
setlocal enabledelayedexpansion

:: ========================================
:: ULTRATHINK BUILD SCRIPT FOR SELACO/ZDOOM
:: WITH VCPKG SUPPORT FOR ARCHIPELAGO
:: ========================================

echo.
echo ===================================================
echo   SELACO BUILD SCRIPT - ULTRATHINK MODE ACTIVATED
echo   NOW WITH VCPKG INTEGRATION!
echo ===================================================
echo.

:: Set build configuration (Release, Debug, RelWithDebInfo, MinSizeRel)
set BUILD_TYPE=Release

:: Set build directory
set BUILD_DIR=build

:: Set number of parallel jobs for compilation (adjust based on your CPU)
set PARALLEL_JOBS=%NUMBER_OF_PROCESSORS%

:: Set vcpkg path (adjust if installed elsewhere)
set VCPKG_ROOT=C:\vcpkg
set VCPKG_TOOLCHAIN=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake

:: Set architecture
set ARCH=x64

:: Check if CMake is available
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] CMake not found in PATH!
    echo Please install CMake from https://cmake.org/download/
    echo.
    pause
    exit /b 1
)

:: Check if vcpkg is available
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo [WARNING] vcpkg not found at %VCPKG_ROOT%
    echo.
    echo To install vcpkg:
    echo   1. git clone https://github.com/Microsoft/vcpkg.git %VCPKG_ROOT%
    echo   2. cd %VCPKG_ROOT%
    echo   3. bootstrap-vcpkg.bat
    echo   4. vcpkg integrate install
    echo.
    set /p CONTINUE_WITHOUT="Continue without vcpkg? (y/n): "
    if /i "!CONTINUE_WITHOUT!" neq "y" (
        exit /b 1
    )
    set USE_VCPKG=0
) else (
    set USE_VCPKG=1
    echo [*] Found vcpkg at %VCPKG_ROOT%
    
    :: Check for required packages
    echo [*] Checking vcpkg packages...
    set MISSING_PACKAGES=
    
    "%VCPKG_ROOT%\vcpkg.exe" list | findstr /C:"jsoncpp:x64-windows" >nul 2>&1
    if %errorlevel% neq 0 (
        set MISSING_PACKAGES=!MISSING_PACKAGES! jsoncpp:x64-windows
    )
    
    "%VCPKG_ROOT%\vcpkg.exe" list | findstr /C:"ixwebsocket:x64-windows" >nul 2>&1
    if %errorlevel% neq 0 (
        set MISSING_PACKAGES=!MISSING_PACKAGES! ixwebsocket:x64-windows
    )
    
    if not "!MISSING_PACKAGES!"=="" (
        echo [WARNING] Missing vcpkg packages:!MISSING_PACKAGES!
        echo.
        set /p INSTALL_PACKAGES="Install missing packages? (y/n): "
        if /i "!INSTALL_PACKAGES!"=="y" (
            echo [*] Installing packages...
            "%VCPKG_ROOT%\vcpkg.exe" install!MISSING_PACKAGES!
            if %errorlevel% neq 0 (
                echo [ERROR] Failed to install packages
                pause
                exit /b 1
            )
        )
    ) else (
        echo [*] All required vcpkg packages are installed
    )
)

:: Clean build directory if requested
if "%1"=="clean" (
    echo [*] Cleaning build directory...
    if exist "%BUILD_DIR%" (
        rmdir /s /q "%BUILD_DIR%"
    )
)

:: Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" (
    echo [*] Creating build directory: %BUILD_DIR%
    mkdir "%BUILD_DIR%"
)

:: Change to build directory
cd "%BUILD_DIR%"

:: Configure with CMake
echo.
echo [*] Configuring project with CMake...
echo     Build Type: %BUILD_TYPE%
echo     Architecture: %ARCH%
if "%USE_VCPKG%"=="1" (
    echo     vcpkg: ENABLED
    echo     Toolchain: %VCPKG_TOOLCHAIN%
) else (
    echo     vcpkg: DISABLED
    echo     mbedTLS: FORCE_INTERNAL_MBEDTLS=ON
)
echo.

:: Build the CMake command
set CMAKE_CMD=cmake .. -G "Visual Studio 17 2022" -A %ARCH% -DCMAKE_BUILD_TYPE=%BUILD_TYPE%

if "%USE_VCPKG%"=="1" (
    :: With vcpkg
    set CMAKE_CMD=!CMAKE_CMD! -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN%"
    set CMAKE_CMD=!CMAKE_CMD! -DVCPKG_TARGET_TRIPLET=x64-windows
) else (
    :: Without vcpkg - use internal mbedTLS
    set CMAKE_CMD=!CMAKE_CMD! -DFORCE_INTERNAL_MBEDTLS=ON
)

:: Add common flags
set CMAKE_CMD=!CMAKE_CMD! -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -Wno-dev

:: Execute CMake
echo [*] Running: !CMAKE_CMD!
echo.
!CMAKE_CMD!

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo [*] Configuration successful!
if "%USE_VCPKG%"=="1" (
    echo [*] Using vcpkg packages for dependencies
) else (
    echo [*] mbedTLS will be built internally for SSL/TLS support
)
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

:: Check if Archipelago support was built
if exist "src\archipelago\archipelago.lib" (
    echo [*] Archipelago support: ENABLED
) else if exist "src\archipelago\%BUILD_TYPE%\archipelago.lib" (
    echo [*] Archipelago support: ENABLED
) else (
    echo [*] Archipelago support: Not detected
)
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
        echo Checking other common locations...
        
        :: Check src folder
        if exist "%BUILD_DIR%\src\%BUILD_TYPE%\zdoom.exe" (
            echo [*] Found in src folder, starting zdoom.exe...
            start "" "%BUILD_DIR%\src\%BUILD_TYPE%\zdoom.exe"
        ) else if exist "%BUILD_DIR%\src\zdoom.exe" (
            echo [*] Found in src folder, starting zdoom.exe...
            start "" "%BUILD_DIR%\src\zdoom.exe"
        ) else (
            echo [ERROR] Could not find zdoom.exe anywhere!
            echo Please locate it manually in the build directory.
        )
    )
)

pause