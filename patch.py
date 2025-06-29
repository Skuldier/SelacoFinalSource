#!/usr/bin/env python3
"""
CMakeLists.txt Autopatcher for Archipelago with vcpkg
This script automatically adds the necessary configuration to your CMakeLists.txt
"""

import os
import re
import shutil
import argparse
from datetime import datetime

class CMakeAutopatcher:
    def __init__(self, cmake_path):
        self.cmake_path = cmake_path
        self.content = ""
        self.main_target = None
        
    def read_file(self):
        """Read the CMakeLists.txt file"""
        with open(self.cmake_path, 'r', encoding='utf-8') as f:
            self.content = f.read()
            
    def backup_file(self):
        """Create a timestamped backup"""
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        backup_path = f"{self.cmake_path}.backup_{timestamp}"
        shutil.copy2(self.cmake_path, backup_path)
        print(f"✓ Created backup: {backup_path}")
        return backup_path
        
    def find_main_target(self):
        """Find the main executable/library target"""
        # Look for add_executable
        exe_pattern = r'add_executable\s*\(\s*([^\s\)]+)'
        exe_matches = re.findall(exe_pattern, self.content)
        
        # Look for add_library that might be the main target
        lib_pattern = r'add_library\s*\(\s*([^\s\)]+)'
        lib_matches = re.findall(lib_pattern, self.content)
        
        # Common game/engine target names
        common_names = ['zdoom', 'gzdoom', 'selaco', 'game', 'engine', 'main']
        
        # Check executables first
        for match in exe_matches:
            if any(name in match.lower() for name in common_names):
                self.main_target = match
                return match
                
        # Check libraries
        for match in lib_matches:
            if any(name in match.lower() for name in common_names):
                self.main_target = match
                return match
                
        # Fall back to first executable
        if exe_matches:
            self.main_target = exe_matches[0]
            return exe_matches[0]
            
        # Fall back to project name
        project_pattern = r'project\s*\(\s*([^\s\)]+)'
        project_matches = re.findall(project_pattern, self.content)
        if project_matches:
            self.main_target = project_matches[0]
            return project_matches[0]
            
        return None
        
    def check_existing_config(self):
        """Check if Archipelago is already configured"""
        indicators = [
            'archipelago',
            'IXWebSocket',
            'jsoncpp CONFIG REQUIRED',
            'ixwebsocket::ixwebsocket'
        ]
        
        return any(indicator in self.content for indicator in indicators)
        
    def find_insertion_point(self):
        """Find the best place to insert the configuration"""
        # Try to find the last find_package
        find_package_pattern = r'(find_package\s*\([^)]+\))'
        matches = list(re.finditer(find_package_pattern, self.content))
        if matches:
            last_match = matches[-1]
            return last_match.end()
            
        # Try after project()
        project_pattern = r'(project\s*\([^)]+\))'
        match = re.search(project_pattern, self.content)
        if match:
            return match.end()
            
        # Try after cmake_minimum_required
        cmake_min_pattern = r'(cmake_minimum_required\s*\([^)]+\))'
        match = re.search(cmake_min_pattern, self.content)
        if match:
            return match.end()
            
        # Default to end of file
        return len(self.content)
        
    def find_target_link_position(self):
        """Find where to add target_link_libraries for archipelago"""
        if not self.main_target:
            return None
            
        # Find existing target_link_libraries for the main target
        pattern = rf'(target_link_libraries\s*\(\s*{re.escape(self.main_target)}[^)]+\))'
        matches = list(re.finditer(pattern, self.content, re.MULTILINE | re.DOTALL))
        
        if matches:
            # Insert after the last one
            return matches[-1].end()
            
        # Find where the target is defined
        target_pattern = rf'(add_(?:executable|library)\s*\(\s*{re.escape(self.main_target)}[^)]+\))'
        match = re.search(target_pattern, self.content, re.MULTILINE | re.DOTALL)
        if match:
            return match.end()
            
        return None
        
    def generate_archipelago_config(self):
        """Generate the Archipelago configuration"""
        config = '''

# ===== Archipelago Support with vcpkg =====
# Added by autopatcher on {}
# Find packages from vcpkg
find_package(jsoncpp CONFIG REQUIRED)
find_package(IXWebSocket CONFIG REQUIRED)

# Archipelago library
if(EXISTS "${{CMAKE_CURRENT_SOURCE_DIR}}/src/archipelago/archipelago_socket.cpp")
    add_library(archipelago STATIC
        src/archipelago/archipelago_socket.cpp
        src/archipelago/archipelago_commands.cpp
    )
    
    # Include directories
    target_include_directories(archipelago PUBLIC
        ${{CMAKE_CURRENT_SOURCE_DIR}}/src/archipelago
    )
    
    # Link libraries from vcpkg
    target_link_libraries(archipelago PUBLIC
        JsonCpp::JsonCpp  # vcpkg provides this target
        ixwebsocket::ixwebsocket
    )
    
    # Windows-specific libraries
    if(WIN32)
        target_link_libraries(archipelago PUBLIC
            ws2_32
            crypt32
            winhttp  # Sometimes needed for SSL
        )
    endif()
    
    message(STATUS "Archipelago support enabled using vcpkg libraries")
    message(STATUS "  JsonCpp: Found")
    message(STATUS "  IXWebSocket: Found")
else()
    message(WARNING "Archipelago source files not found in src/archipelago/")
endif()

'''.format(datetime.now().strftime('%Y-%m-%d %H:%M:%S'))
        
        return config
        
    def generate_target_link(self):
        """Generate the target_link_libraries line"""
        if not self.main_target:
            return ""
            
        return f'''
# Link Archipelago to main target
if(TARGET archipelago)
    target_link_libraries({self.main_target} PRIVATE archipelago)
endif()
'''
        
    def apply_patch(self):
        """Apply the patch to CMakeLists.txt"""
        # Insert main configuration
        insertion_point = self.find_insertion_point()
        config = self.generate_archipelago_config()
        
        # Insert the configuration
        new_content = (
            self.content[:insertion_point] + 
            config + 
            self.content[insertion_point:]
        )
        
        # Add target linking if we found a main target
        if self.main_target:
            link_position = self.find_target_link_position()
            if link_position:
                target_link = self.generate_target_link()
                # Adjust position for the already inserted config
                adjusted_position = link_position + len(config)
                new_content = (
                    new_content[:adjusted_position] + 
                    target_link + 
                    new_content[adjusted_position:]
                )
        
        # Write the modified content
        with open(self.cmake_path, 'w', encoding='utf-8') as f:
            f.write(new_content)
            
        print(f"✓ Successfully patched {self.cmake_path}")
        
    def run(self):
        """Run the autopatcher"""
        print("Archipelago CMakeLists.txt Autopatcher")
        print("=" * 50)
        
        # Read the file
        self.read_file()
        print(f"✓ Read {self.cmake_path}")
        
        # Check if already configured
        if self.check_existing_config():
            print("\n⚠ Warning: CMakeLists.txt appears to already have Archipelago configuration.")
            response = input("Continue anyway? (y/n): ")
            if response.lower() != 'y':
                print("Aborted.")
                return False
                
        # Find main target
        target = self.find_main_target()
        if target:
            print(f"✓ Found main target: {target}")
        else:
            print("⚠ Could not detect main target automatically")
            target = input("Enter your main target name (or press Enter to skip): ").strip()
            if target:
                self.main_target = target
                
        # Create backup
        self.backup_file()
        
        # Apply patch
        self.apply_patch()
        
        print("\n" + "=" * 50)
        print("✅ Patching complete!")
        print("\nNext steps:")
        print("1. Make sure vcpkg is installed and integrated")
        print("2. Run: vcpkg install jsoncpp:x64-windows ixwebsocket:x64-windows")
        print("3. Configure CMake with vcpkg toolchain:")
        print('   cmake -B build -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"')
        print("4. Build your project")
        
        return True

def find_cmake_file(start_path='.'):
    """Find CMakeLists.txt in current or parent directories"""
    current = os.path.abspath(start_path)
    while current != os.path.dirname(current):
        cmake_path = os.path.join(current, 'CMakeLists.txt')
        if os.path.exists(cmake_path):
            return cmake_path
        current = os.path.dirname(current)
    return None

def main():
    parser = argparse.ArgumentParser(
        description='Automatically patch CMakeLists.txt for Archipelago support with vcpkg'
    )
    parser.add_argument(
        'cmake_file', 
        nargs='?', 
        help='Path to CMakeLists.txt (auto-detected if not specified)'
    )
    parser.add_argument(
        '--target', '-t',
        help='Main target name to link archipelago with'
    )
    args = parser.parse_args()
    
    # Find CMakeLists.txt
    if args.cmake_file:
        cmake_path = args.cmake_file
    else:
        cmake_path = find_cmake_file()
        if not cmake_path:
            print("❌ Could not find CMakeLists.txt in current or parent directories.")
            cmake_path = input("Enter path to CMakeLists.txt: ").strip()
            
    if not os.path.exists(cmake_path):
        print(f"❌ Error: {cmake_path} does not exist")
        return 1
        
    # Run the patcher
    patcher = CMakeAutopatcher(cmake_path)
    
    if args.target:
        patcher.main_target = args.target
        
    try:
        success = patcher.run()
        return 0 if success else 1
    except Exception as e:
        print(f"❌ Error: {e}")
        return 1

if __name__ == '__main__':
    exit(main())