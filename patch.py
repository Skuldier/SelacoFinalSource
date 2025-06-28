#!/usr/bin/env python3
"""
Archipelago Integration Auto-Patcher for Selaco
This script automatically applies the necessary patches to integrate Archipelago support.
"""

import os
import re
import shutil
from datetime import datetime

def backup_file(filepath):
    """Create a backup of the file before modifying it."""
    backup_path = f"{filepath}.backup_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
    shutil.copy2(filepath, backup_path)
    print(f"Created backup: {backup_path}")
    return backup_path

def patch_d_main_cpp(filepath):
    """Apply patches to d_main.cpp"""
    if not os.path.exists(filepath):
        print(f"Error: {filepath} not found!")
        return False
    
    # Read the file
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    original_content = content
    patches_applied = []
    
    # Patch 1: Remove duplicate include
    duplicate_include = r'#include "archipelago/archipelago_integration\.h"\s*//\s*for SHARE_DIR'
    if re.search(duplicate_include, content):
        content = re.sub(duplicate_include, '', content)
        patches_applied.append("Removed duplicate archipelago include")
    
    # Patch 2: Add Archipelago_ProcessMessages to D_DoomLoop
    # Check if it's already there
    if 'Archipelago_ProcessMessages()' not in content:
        # Find the location to insert
        pattern = r'(TryRunTics \(\); // will run at least one tic\s*\}\s*)(// Update display.*?\s*I_StartTic \(\);)'
        match = re.search(pattern, content, re.DOTALL)
        if match:
            replacement = match.group(1) + '// Process Archipelago messages\n\t\t\tArchipelago_ProcessMessages();\n\t\t\t' + match.group(2)
            content = content[:match.start()] + replacement + content[match.end():]
            patches_applied.append("Added Archipelago_ProcessMessages() to main loop")
        else:
            print("Warning: Could not find location to add Archipelago_ProcessMessages()")
    
    # Patch 3: Add Archipelago_Shutdown to D_Cleanup
    if 'Archipelago_Shutdown()' not in content:
        # Find DeleteScreenJob() and add after it
        pattern = r'(DeleteScreenJob\(\);)'
        match = re.search(pattern, content)
        if match:
            replacement = match.group(1) + '\n\t\n\t// Shutdown Archipelago\n\tArchipelago_Shutdown();'
            content = content[:match.start()] + replacement + content[match.end():]
            patches_applied.append("Added Archipelago_Shutdown() to cleanup")
        else:
            print("Warning: Could not find location to add Archipelago_Shutdown()")
    
    # Only write if changes were made
    if content != original_content:
        backup_file(filepath)
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"\nSuccessfully patched {filepath}")
        print("Patches applied:")
        for patch in patches_applied:
            print(f"  - {patch}")
        return True
    else:
        print(f"{filepath} is already patched or doesn't need patches")
        return True

def verify_cmake_lists(filepath):
    """Verify that CMakeLists.txt has Archipelago sources included"""
    if not os.path.exists(filepath):
        print(f"Error: {filepath} not found!")
        return False
    
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Check for Archipelago sources
    if 'ARCHIPELAGO_SOURCES' in content and 'archipelago/archipelago_socket.cpp' in content:
        print(f"✓ {filepath} already has Archipelago integration")
        return True
    else:
        print(f"✗ {filepath} is missing Archipelago integration!")
        print("Please add the following to your CMakeLists.txt:")
        print("""
# Archipelago integration sources
set(ARCHIPELAGO_SOURCES
    archipelago/archipelago_socket.cpp
    archipelago/archipelago_commands.cpp
    archipelago/archipelago_selaco.cpp
)

set(ARCHIPELAGO_HEADERS
    archipelago/archipelago_socket.h
    archipelago/archipelago_integration.h
    archipelago/archipelago_items.h
)

# And add ${ARCHIPELAGO_SOURCES} to your GAME_SOURCES
""")
        return False

def main():
    print("=== Archipelago Integration Auto-Patcher for Selaco ===\n")
    
    # Check if we're in the right directory
    if not os.path.exists('src'):
        print("Error: 'src' directory not found!")
        print("Please run this script from the Selaco project root directory.")
        return
    
    # Verify CMakeLists.txt
    print("Checking CMakeLists.txt...")
    cmake_ok = verify_cmake_lists('src/CMakeLists.txt')
    
    # Patch d_main.cpp
    print("\nPatching d_main.cpp...")
    d_main_ok = patch_d_main_cpp('src/d_main.cpp')
    
    # Create archipelago directory if it doesn't exist
    archipelago_dir = 'src/archipelago'
    if not os.path.exists(archipelago_dir):
        os.makedirs(archipelago_dir)
        print(f"\nCreated directory: {archipelago_dir}")
        print("Don't forget to copy all the Archipelago source files to this directory!")
    
    # Summary
    print("\n=== Summary ===")
    if cmake_ok and d_main_ok:
        print("✓ All patches applied successfully!")
        print("\nNext steps:")
        print("1. Copy all archipelago/*.cpp and archipelago/*.h files to src/archipelago/")
        print("2. Rebuild your project")
        print("3. Test with 'archipelago_connect' console command")
    else:
        print("✗ Some patches failed. Please check the messages above.")

if __name__ == "__main__":
    main()