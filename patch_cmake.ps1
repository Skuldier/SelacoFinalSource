# =====================================================
# ULTRATHINK CMAKE AUTOPATCHER FOR ARCHIPELAGO SUPPORT
# PowerShell Version - More robust file handling
# =====================================================

Write-Host ""
Write-Host "========================================================" -ForegroundColor Cyan
Write-Host "  CMakeLists.txt AUTOPATCHER - ARCHIPELAGO INTEGRATION" -ForegroundColor Cyan
Write-Host "========================================================" -ForegroundColor Cyan
Write-Host ""

# Configuration
$cmakeFile = "src\CMakeLists.txt"
$backupFile = "src\CMakeLists.txt.backup"

# Check if file exists
if (-not (Test-Path $cmakeFile)) {
    Write-Host "[ERROR] $cmakeFile not found!" -ForegroundColor Red
    Write-Host "Please run this from the root directory of your project." -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit 1
}

# Check if already patched
$content = Get-Content $cmakeFile -Raw
if ($content -match "NO_ARCHIPELAGO") {
    Write-Host "[INFO] CMakeLists.txt appears to already have Archipelago support!" -ForegroundColor Yellow
    $response = Read-Host "Do you want to re-apply the patch? (y/n)"
    if ($response -ne 'y') {
        Write-Host "[INFO] Patch cancelled." -ForegroundColor Yellow
        Read-Host "Press Enter to exit"
        exit 0
    }
}

# Create backup
Write-Host "[*] Creating backup: $backupFile" -ForegroundColor Green
Copy-Item $cmakeFile $backupFile -Force

# The patch content
$archipelagoPatch = @'
# Archipelago support option
option(NO_ARCHIPELAGO "Disable Archipelago multiworld support" OFF)

if(NOT NO_ARCHIPELAGO)
    # Check if source files exist
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/archipelago/archipelago_socket.cpp")
        # Try to find jsoncpp
        find_package(jsoncpp CONFIG QUIET)
        if(NOT jsoncpp_FOUND)
            find_package(PkgConfig QUIET)
            if(PkgConfig_FOUND)
                pkg_check_modules(JSONCPP jsoncpp)
            endif()
        endif()

        # Try to find ixwebsocket
        find_package(ixwebsocket CONFIG QUIET)
        
        if(jsoncpp_FOUND OR JSONCPP_FOUND)
            message(STATUS "Found jsoncpp for Archipelago support")
            
            # Set up jsoncpp includes and libraries
            if(jsoncpp_FOUND)
                # vcpkg style
                set(ARCHIPELAGO_LIBRARIES jsoncpp_lib)
            elseif(JSONCPP_FOUND)
                # pkg-config style
                set(ARCHIPELAGO_INCLUDE_DIRS ${JSONCPP_INCLUDE_DIRS})
                set(ARCHIPELAGO_LIBRARIES ${JSONCPP_LIBRARIES})
            endif()
            
            # Add ixwebsocket if found
            if(ixwebsocket_FOUND)
                message(STATUS "Found ixwebsocket for Archipelago support")
                list(APPEND ARCHIPELAGO_LIBRARIES ixwebsocket::ixwebsocket)
            else()
                message(WARNING "ixwebsocket not found - Archipelago WebSocket support may be limited")
            endif()
            
            # Define Archipelago sources
            set(ARCHIPELAGO_SOURCES
                archipelago/archipelago_socket.cpp
                archipelago/archipelago_commands.cpp
            )

            set(ARCHIPELAGO_HEADERS
                archipelago/archipelago_socket.h
                archipelago/archipelago_integration.h
            )
            
            # Add to the build
            set(ARCHIPELAGO_ENABLED TRUE)
            add_definitions(-DARCHIPELAGO_SUPPORT)
            
        else()
            message(WARNING "jsoncpp not found - Archipelago support will be disabled")
            message(WARNING "To enable Archipelago support, install jsoncpp via vcpkg:")
            message(WARNING "  vcpkg install jsoncpp:x64-windows ixwebsocket:x64-windows")
            set(ARCHIPELAGO_SOURCES "")
            set(ARCHIPELAGO_HEADERS "")
            set(ARCHIPELAGO_ENABLED FALSE)
        endif()
    else()
        message(WARNING "Archipelago source files not found in src/archipelago/")
        set(ARCHIPELAGO_SOURCES "")
        set(ARCHIPELAGO_HEADERS "")
        set(ARCHIPELAGO_ENABLED FALSE)
    endif()
else()
    message(STATUS "Archipelago support disabled by NO_ARCHIPELAGO option")
    set(ARCHIPELAGO_SOURCES "")
    set(ARCHIPELAGO_HEADERS "")
    set(ARCHIPELAGO_ENABLED FALSE)
endif()
'@

$libraryPatch = @'

# Add Archipelago libraries if enabled
if(ARCHIPELAGO_ENABLED)
    list(APPEND PROJECT_LIBRARIES ${ARCHIPELAGO_LIBRARIES})
    if(ARCHIPELAGO_INCLUDE_DIRS)
        include_directories(${ARCHIPELAGO_INCLUDE_DIRS})
    endif()
endif()
'@

Write-Host "[*] Processing CMakeLists.txt..." -ForegroundColor Green

# Read the file
$lines = Get-Content $cmakeFile

# Process the file
$output = @()
$inArchipelagoSection = $false
$patchApplied = $false
$foundProjectLibraries = $false
$skipNextEmptyLine = $false

for ($i = 0; $i -lt $lines.Count; $i++) {
    $line = $lines[$i]
    
    # Skip old ARCHIPELAGO_SOURCES section
    if ($line -match "set\s*\(\s*ARCHIPELAGO_SOURCES" -or $line -match "ARCHIPELAGO_SOURCES\s*=" -or 
        ($line -match "# Archipelago integration sources" -and $i+1 -lt $lines.Count -and $lines[$i+1] -match "ARCHIPELAGO_SOURCES")) {
        
        if (-not $patchApplied) {
            # Insert our patch
            $output += $archipelagoPatch
            $output += ""
            $patchApplied = $true
        }
        
        # Skip until we find the closing parenthesis
        $inArchipelagoSection = $true
        $skipNextEmptyLine = $true
        continue
    }
    
    # Skip old ARCHIPELAGO_HEADERS section
    if ($line -match "set\s*\(\s*ARCHIPELAGO_HEADERS") {
        # Skip until closing parenthesis
        while ($i -lt $lines.Count -and $lines[$i] -notmatch "\)") {
            $i++
        }
        continue
    }
    
    # If we're in the old section, skip lines until the closing
    if ($inArchipelagoSection) {
        if ($line -match "\)") {
            $inArchipelagoSection = $false
        }
        continue
    }
    
    # Skip empty lines after removed sections
    if ($skipNextEmptyLine -and $line.Trim() -eq "") {
        $skipNextEmptyLine = $false
        continue
    }
    
    # Add the line
    $output += $line
    
    # Check if this is where we should add the patch (before GAME_SOURCES if not already added)
    if (-not $patchApplied -and $line -match "set\s*\(\s*GAME_SOURCES") {
        # Insert patch before GAME_SOURCES
        $insertIndex = $output.Count - 1
        $beforeGameSources = $output[0..($insertIndex-1)]
        $fromGameSources = $output[$insertIndex..($output.Count-1)]
        
        $output = $beforeGameSources
        $output += ""
        $output += $archipelagoPatch
        $output += ""
        $output += $fromGameSources
        
        $patchApplied = $true
    }
    
    # Add library configuration after PROJECT_LIBRARIES
    if ($line -match "set\s*\(\s*PROJECT_LIBRARIES" -and -not $foundProjectLibraries) {
        # Find the closing parenthesis for PROJECT_LIBRARIES
        $j = $i
        while ($j -lt $lines.Count -and $lines[$j] -notmatch "\)") {
            $j++
            if ($j -lt $lines.Count) {
                $output += $lines[$j]
            }
        }
        $i = $j
        
        # Add our library patch
        $output += $libraryPatch
        $foundProjectLibraries = $true
    }
}

# Write the patched content
Write-Host "[*] Writing patched CMakeLists.txt..." -ForegroundColor Green
$output | Set-Content $cmakeFile -Encoding UTF8

Write-Host ""
Write-Host "[SUCCESS] CMakeLists.txt has been patched!" -ForegroundColor Green
Write-Host ""
Write-Host "[*] Backup saved as: $backupFile" -ForegroundColor Cyan
Write-Host "[*] Archipelago support has been added" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Install required packages: vcpkg install jsoncpp:x64-windows ixwebsocket:x64-windows" -ForegroundColor White
Write-Host "  2. Run build_selaco.bat to build with Archipelago support" -ForegroundColor White
Write-Host ""
Write-Host "If something went wrong, restore from: $backupFile" -ForegroundColor Yellow
Write-Host ""
Read-Host "Press Enter to exit"