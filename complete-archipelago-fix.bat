@echo off
setlocal enabledelayedexpansion

:: =====================================================
:: SIMPLE VCPKG.JSON FIX FOR ARCHIPELAGO
:: =====================================================

echo.
echo ========================================================
echo   FIXING VCPKG.JSON FOR ARCHIPELAGO
echo ========================================================
echo.

:: Check if vcpkg.json exists
if not exist "vcpkg.json" (
    echo [ERROR] vcpkg.json not found!
    pause
    exit /b 1
)

:: Create backup
echo [*] Creating backup...
copy vcpkg.json vcpkg.json.backup >nul 2>&1

:: Show current content
echo [*] Current vcpkg.json:
echo ----------------------------------------
type vcpkg.json
echo ----------------------------------------
echo.

:: Create a PowerShell script to do the update
echo [*] Creating update script...
(
echo $json = Get-Content 'vcpkg.json' -Raw ^| ConvertFrom-Json
echo.
echo # Ensure dependencies array exists
echo if ^(-not $json.PSObject.Properties['dependencies'^]^) {
echo     $json ^| Add-Member -Name 'dependencies' -Value @^(^) -MemberType NoteProperty
echo }
echo.
echo # Convert to array if string
echo $deps = $json.dependencies
echo if ^($deps -is [string^]^) {
echo     $deps = @^($deps^)
echo }
echo.
echo # Add missing dependencies
echo $added = $false
echo if ^($deps -notcontains 'jsoncpp'^) {
echo     $deps += 'jsoncpp'
echo     Write-Host '[+] Added jsoncpp'
echo     $added = $true
echo }
echo if ^($deps -notcontains 'ixwebsocket'^) {
echo     $deps += 'ixwebsocket'  
echo     Write-Host '[+] Added ixwebsocket'
echo     $added = $true
echo }
echo.
echo # Update and save
echo $json.dependencies = $deps
echo $json ^| ConvertTo-Json -Depth 10 ^| Set-Content 'vcpkg.json' -Encoding UTF8
echo.
echo if ^($added^) {
echo     Write-Host '[OK] vcpkg.json updated successfully'
echo } else {
echo     Write-Host '[OK] vcpkg.json already has required dependencies'
echo }
) > update_vcpkg.ps1

:: Run the PowerShell script
echo [*] Updating vcpkg.json...
powershell -ExecutionPolicy Bypass -File update_vcpkg.ps1

:: Clean up
del update_vcpkg.ps1 >nul 2>&1

:: Show updated content
echo.
echo [*] Updated vcpkg.json:
echo ----------------------------------------
type vcpkg.json
echo ----------------------------------------

:: Install dependencies
echo.
echo [*] Installing dependencies from manifest...
vcpkg install

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] vcpkg install failed!
    pause
    exit /b 1
)

echo.
echo ========================================================
echo   SUCCESS!
echo ========================================================
echo.
echo Dependencies have been added and installed.
echo.
echo Next steps:
echo   1. Clean build: rmdir /s /q build
echo   2. Run: build_selaco.bat
echo.
pause