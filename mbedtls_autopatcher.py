#!/usr/bin/env python3
"""
mbedTLS Auto-Patcher for GZDoom/APDoom
Automatically integrates mbedTLS into the CMake build system
"""

import os
import re
import shutil
import subprocess
import sys
import argparse
from datetime import datetime
from pathlib import Path

class MbedTLSAutoPatcher:
    def __init__(self, project_root='.', skip_submodule=False):
        self.project_root = Path(project_root)
        self.backup_dir = self.project_root / '.autopatcher_backups' / datetime.now().strftime('%Y%m%d_%H%M%S')
        self.patches_applied = []
        self.errors = []
        self.skip_submodule = skip_submodule
        
    def backup_file(self, filepath):
        """Create a backup of a file before modifying it"""
        if not filepath.exists():
            return False
            
        backup_path = self.backup_dir / filepath.relative_to(self.project_root)
        backup_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(filepath, backup_path)
        return True
        
    def check_patch_exists(self, content, marker):
        """Check if a patch has already been applied"""
        return marker in content
        
    def patch_root_cmake(self):
        """Patch the root CMakeLists.txt file"""
        cmake_path = self.project_root / 'CMakeLists.txt'
        if not cmake_path.exists():
            self.errors.append(f"Root CMakeLists.txt not found at {cmake_path}")
            return False
            
        print("📝 Patching root CMakeLists.txt...")
        
        # Read the file
        with open(cmake_path, 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Check if already patched
        if self.check_patch_exists(content, 'FORCE_INTERNAL_MBEDTLS'):
            print("   ✓ Already patched, skipping...")
            return True
            
        # Backup the file
        self.backup_file(cmake_path)
        
        # Find insertion point - after other library options
        # Look for patterns like "option(FORCE_INTERNAL_" or similar library options
        insert_pattern = r'(option\s*\(\s*FORCE_INTERNAL_\w+.*?\).*?)\n'
        match = re.search(insert_pattern, content, re.DOTALL | re.MULTILINE)
        
        if not match:
            # Alternative: look for any option() statement near the beginning
            insert_pattern = r'(option\s*\([^)]+\).*?)\n(?=\n)'
            match = re.search(insert_pattern, content, re.DOTALL | re.MULTILINE)
            
        if not match:
            # Fallback: insert after project() statement
            insert_pattern = r'(project\s*\([^)]+\).*?)\n'
            match = re.search(insert_pattern, content, re.DOTALL | re.MULTILINE)
            
        if match:
            insert_pos = match.end()
            
            mbedtls_config = '''
# mbedTLS configuration
option(FORCE_INTERNAL_MBEDTLS "Use internal mbedTLS" ON)
mark_as_advanced(FORCE_INTERNAL_MBEDTLS)

# Check for system mbedTLS first if not forcing internal
if(NOT FORCE_INTERNAL_MBEDTLS)
    find_package(MbedTLS)
endif()

if(MBEDTLS_FOUND AND NOT FORCE_INTERNAL_MBEDTLS)
    message(STATUS "Using system mbedTLS library")
    set(MBEDTLS_LIBRARIES mbedtls mbedcrypto mbedx509)
else()
    message(STATUS "Using internal mbedTLS library")
    
    # Configure mbedTLS options for minimal build
    set(ENABLE_TESTING OFF CACHE BOOL "Build mbedTLS tests")
    set(ENABLE_PROGRAMS OFF CACHE BOOL "Build mbedTLS programs")
    set(UNSAFE_BUILD OFF CACHE BOOL "Allow unsafe builds")
    
    # Add mbedTLS subdirectory
    add_subdirectory(libraries/mbedtls EXCLUDE_FROM_ALL)
    
    # Set up variables for our project
    set(MBEDTLS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libraries/mbedtls/include" CACHE INTERNAL "")
    set(MBEDTLS_LIBRARIES mbedtls mbedcrypto mbedx509 CACHE INTERNAL "")
endif()
'''
            
            new_content = content[:insert_pos] + mbedtls_config + content[insert_pos:]
            
            # Write the patched content
            with open(cmake_path, 'w', encoding='utf-8') as f:
                f.write(new_content)
                
            self.patches_applied.append("Root CMakeLists.txt")
            print("   ✓ Successfully patched!")
            return True
        else:
            self.errors.append("Could not find suitable insertion point in root CMakeLists.txt")
            return False
            
    def patch_src_cmake(self):
        """Patch the src/CMakeLists.txt file"""
        cmake_path = self.project_root / 'src' / 'CMakeLists.txt'
        if not cmake_path.exists():
            self.errors.append(f"src/CMakeLists.txt not found at {cmake_path}")
            return False
            
        print("📝 Patching src/CMakeLists.txt...")
        
        # Read the file
        with open(cmake_path, 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Check if already patched
        if self.check_patch_exists(content, 'MBEDTLS_LIBRARIES'):
            print("   ✓ Already patched, skipping...")
            return True
            
        # Backup the file
        self.backup_file(cmake_path)
        
        patches_made = False
        
        # Patch 1: Add mbedTLS to PROJECT_LIBRARIES
        # Find where PROJECT_LIBRARIES is being set
        lib_pattern = r'(set\s*\(\s*PROJECT_LIBRARIES[^)]+\))'
        match = re.search(lib_pattern, content, re.DOTALL)
        
        if match:
            insert_pos = match.end()
            
            mbedtls_libs = '''

# Add mbedTLS to project libraries
list(APPEND PROJECT_LIBRARIES ${MBEDTLS_LIBRARIES})
'''
            
            content = content[:insert_pos] + mbedtls_libs + content[insert_pos:]
            patches_made = True
            
        # Patch 2: Add mbedTLS include directory
        # Find include_directories section
        include_pattern = r'(include_directories\s*\([^)]*SYSTEM[^)]*\))'
        matches = list(re.finditer(include_pattern, content, re.DOTALL))
        
        if matches:
            # Insert after the last include_directories with SYSTEM
            last_match = matches[-1]
            insert_pos = last_match.end()
            
            mbedtls_include = '''

# Add mbedTLS include directory
include_directories(SYSTEM ${MBEDTLS_INCLUDE_DIR})
'''
            
            content = content[:insert_pos] + mbedtls_include + content[insert_pos:]
            patches_made = True
            
        # Patch 3: Add Windows-specific libraries for MSVC
        # Find the MSVC section
        msvc_pattern = r'(if\s*\(\s*MSVC\s*\).*?endif\s*\(\s*\))'
        matches = list(re.finditer(msvc_pattern, content, re.DOTALL))
        
        if matches:
            # Find a suitable MSVC block (preferably one that adds libraries)
            for match in matches:
                block_content = match.group(1)
                if 'PROJECT_LIBRARIES' in block_content or 'link_libraries' in block_content:
                    # Insert before the endif()
                    endif_pos = match.end() - len('endif()')
                    
                    msvc_libs = '''    # mbedTLS needs these Windows libraries
    list(APPEND PROJECT_LIBRARIES ws2_32 bcrypt)
'''
                    
                    content = content[:endif_pos] + msvc_libs + '\n' + content[endif_pos:]
                    patches_made = True
                    break
                    
        if patches_made:
            # Write the patched content
            with open(cmake_path, 'w', encoding='utf-8') as f:
                f.write(content)
                
            self.patches_applied.append("src/CMakeLists.txt")
            print("   ✓ Successfully patched!")
            return True
        else:
            self.errors.append("Could not find suitable insertion points in src/CMakeLists.txt")
            return False
            
    def patch_archipelago_cmake(self):
        """Patch the src/archipelago/CMakeLists.txt file"""
        cmake_path = self.project_root / 'src' / 'archipelago' / 'CMakeLists.txt'
        if not cmake_path.exists():
            self.errors.append(f"src/archipelago/CMakeLists.txt not found at {cmake_path}")
            return False
            
        print("📝 Patching src/archipelago/CMakeLists.txt...")
        
        # Read the file
        with open(cmake_path, 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Check if already patched
        if self.check_patch_exists(content, 'MBEDTLS_INCLUDE_DIR'):
            print("   ✓ Already patched, skipping...")
            return True
            
        # Backup the file
        self.backup_file(cmake_path)
        
        # Find the target_link_libraries line
        link_pattern = r'(target_link_libraries\s*\(\s*\$\{PROJECT_NAME\}[^)]*\))'
        match = re.search(link_pattern, content, re.DOTALL)
        
        if match:
            # Replace the existing target_link_libraries with an expanded version
            old_link = match.group(1)
            
            # Extract existing libraries
            existing_libs_match = re.search(r'target_link_libraries\s*\(\s*\$\{PROJECT_NAME\}\s+(.+)\)', old_link, re.DOTALL)
            if existing_libs_match:
                existing_libs = existing_libs_match.group(1).strip()
            else:
                existing_libs = ""
                
            new_link = f'''target_link_libraries(${{PROJECT_NAME}} 
    PUBLIC 
    {existing_libs}
    ${{MBEDTLS_LIBRARIES}}
)'''
            
            content = content.replace(old_link, new_link)
            
            # Also update target_include_directories
            include_pattern = r'(target_include_directories\s*\(\s*\$\{PROJECT_NAME\}[^)]*\))'
            include_match = re.search(include_pattern, content, re.DOTALL)
            
            if include_match:
                old_include = include_match.group(1)
                
                # Extract existing directories
                dirs_match = re.search(r'PRIVATE\s+(.+?)(?:\s*\))', old_include, re.DOTALL)
                if dirs_match:
                    existing_dirs = dirs_match.group(1).strip()
                else:
                    existing_dirs = ""
                    
                new_include = f'''target_include_directories(${{PROJECT_NAME}} 
    PRIVATE 
    {existing_dirs}
    ${{MBEDTLS_INCLUDE_DIR}}
)'''
                
                content = content.replace(old_include, new_include)
                
            # Write the patched content
            with open(cmake_path, 'w', encoding='utf-8') as f:
                f.write(content)
                
            self.patches_applied.append("src/archipelago/CMakeLists.txt")
            print("   ✓ Successfully patched!")
            return True
        else:
            self.errors.append("Could not find target_link_libraries in src/archipelago/CMakeLists.txt")
            return False
            
    def add_git_submodule(self):
        """Add mbedTLS as a git submodule"""
        # Check if submodule already exists
        submodule_path = self.project_root / 'libraries' / 'mbedtls'
        if submodule_path.exists():
            print("📦 mbedTLS submodule already exists")
            print("   ✓ Skipping git submodule setup...")
            return True
            
        if self.skip_submodule:
            print("📦 Skipping git submodule setup (--skip-submodule flag)")
            return True
            
        print("📦 Adding mbedTLS as git submodule...")
        
        # Check if .git directory exists
        if not (self.project_root / '.git').exists():
            self.errors.append("Not a git repository. Please initialize git first.")
            return False
            
        try:
            # Create libraries directory if it doesn't exist
            libraries_dir = self.project_root / 'libraries'
            libraries_dir.mkdir(exist_ok=True)
            
            # Add submodule
            subprocess.run([
                'git', 'submodule', 'add', 
                'https://github.com/Mbed-TLS/mbedtls.git', 
                'libraries/mbedtls'
            ], cwd=self.project_root, check=True)
            
            # Checkout stable version
            subprocess.run([
                'git', 'checkout', 'v3.5.1'
            ], cwd=submodule_path, check=True)
            
            # Stage the changes
            subprocess.run([
                'git', 'add', '.gitmodules', 'libraries/mbedtls'
            ], cwd=self.project_root, check=True)
            
            print("   ✓ Successfully added mbedTLS submodule!")
            print("   📌 Note: You still need to commit these changes:")
            print("      git commit -m \"Add mbedTLS as submodule for SSL/TLS support\"")
            
            return True
            
        except subprocess.CalledProcessError as e:
            self.errors.append(f"Failed to add git submodule: {e}")
            return False
            
    def run(self):
        """Run all patches"""
        print("🚀 mbedTLS Auto-Patcher for GZDoom/APDoom")
        print("=" * 50)
        
        if self.skip_submodule:
            print("ℹ️  Git submodule setup will be skipped")
            print()
        
        # Check if we're in the right directory
        if not (self.project_root / 'CMakeLists.txt').exists():
            print("❌ Error: CMakeLists.txt not found in current directory.")
            print("   Please run this script from the project root directory.")
            return False
            
        # Create backup directory
        self.backup_dir.mkdir(parents=True, exist_ok=True)
        print(f"📁 Backups will be saved to: {self.backup_dir.relative_to(self.project_root)}")
        print()
        
        # Run patches
        success = True
        
        # Patch CMake files
        success &= self.patch_root_cmake()
        success &= self.patch_src_cmake()
        success &= self.patch_archipelago_cmake()
        
        # Add git submodule
        success &= self.add_git_submodule()
        
        # Summary
        print()
        print("=" * 50)
        print("📊 Summary:")
        
        if self.patches_applied:
            print(f"✅ Successfully patched {len(self.patches_applied)} file(s):")
            for patch in self.patches_applied:
                print(f"   - {patch}")
                
        if self.errors:
            print(f"❌ Encountered {len(self.errors)} error(s):")
            for error in self.errors:
                print(f"   - {error}")
                
        if success and not self.errors:
            print()
            print("🎉 All patches applied successfully!")
            print()
            print("📋 Next steps:")
            print("   1. Review the changes")
            print("   2. Commit the changes: git commit -m \"Add mbedTLS support\"")
            print("   3. Rebuild your project with CMake")
            return True
        else:
            print()
            print("⚠️  Some patches failed. Please check the errors above.")
            return False

def main():
    """Main entry point"""
    import argparse
    
    # Set up argument parser
    parser = argparse.ArgumentParser(
        description='mbedTLS Auto-Patcher for GZDoom/APDoom',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  %(prog)s                     # Patch current directory
  %(prog)s /path/to/project    # Patch specific directory
  %(prog)s --skip-submodule    # Skip git submodule setup
  %(prog)s --cmake-only        # Only patch CMake files
        '''
    )
    
    parser.add_argument(
        'project_root',
        nargs='?',
        default='.',
        help='Project root directory (default: current directory)'
    )
    
    parser.add_argument(
        '--skip-submodule',
        action='store_true',
        help='Skip git submodule setup (if you already have mbedTLS)'
    )
    
    parser.add_argument(
        '--cmake-only',
        action='store_true',
        help='Only patch CMake files (alias for --skip-submodule)'
    )
    
    args = parser.parse_args()
    
    # Handle cmake-only flag
    skip_submodule = args.skip_submodule or args.cmake_only
    
    # Run the patcher
    patcher = MbedTLSAutoPatcher(args.project_root, skip_submodule)
    success = patcher.run()
    
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()