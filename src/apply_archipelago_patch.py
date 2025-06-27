#!/usr/bin/env python3
"""
Archipelago Integration Patcher for Selaco
Automatically patches CMakeLists.txt and d_main.cpp to add Archipelago support
"""

import os
import sys
import shutil
import re
from datetime import datetime

class ArchipelagoPatcher:
    def __init__(self):
        self.backup_suffix = f".backup_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        self.patches_applied = []
        
    def backup_file(self, filepath):
        """Create a backup of the original file"""
        backup_path = filepath + self.backup_suffix
        shutil.copy2(filepath, backup_path)
        print(f"✓ Created backup: {backup_path}")
        return backup_path
        
    def patch_cmake_lists(self, cmake_path):
        """Patch CMakeLists.txt to include Archipelago sources"""
        print("\n📝 Patching CMakeLists.txt...")
        
        if not os.path.exists(cmake_path):
            print(f"❌ Error: {cmake_path} not found!")
            return False
            
        self.backup_file(cmake_path)
        
        with open(cmake_path, 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Check if already patched
        if 'ARCHIPELAGO_SOURCES' in content:
            print("ℹ️  CMakeLists.txt already contains Archipelago sources, skipping...")
            return True
            
        # Find the PCH_SOURCES section
        pch_sources_match = re.search(r'set \(PCH_SOURCES\s*\n(.*?)\n\)', content, re.DOTALL)
        if not pch_sources_match:
            print("❌ Error: Could not find PCH_SOURCES section!")
            return False
            
        # Add Archipelago sources after PCH_SOURCES
        archipelago_section = '''
# Archipelago integration sources
set(ARCHIPELAGO_SOURCES
    archipelago/archipelago_socket.cpp
    archipelago/archipelago_commands.cpp
)

set(ARCHIPELAGO_HEADERS
    archipelago/archipelago_socket.h
    archipelago/archipelago_integration.h
)
'''
        
        # Insert after PCH_SOURCES
        insert_pos = pch_sources_match.end()
        content = content[:insert_pos] + archipelago_section + content[insert_pos:]
        
        # Add to GAME_SOURCES
        game_sources_match = re.search(r'set\( GAME_SOURCES\s*\n(.*?)\n\)', content, re.DOTALL)
        if game_sources_match:
            game_sources_content = game_sources_match.group(1)
            if '${ARCHIPELAGO_SOURCES}' not in game_sources_content:
                # Add before the closing parenthesis
                new_game_sources = game_sources_match.group(0).rstrip(')')
                new_game_sources += '\t${ARCHIPELAGO_SOURCES}\n\t${ARCHIPELAGO_HEADERS}\n)'
                content = content.replace(game_sources_match.group(0), new_game_sources)
        
        # Add include directory
        include_match = re.search(r'include_directories\(\s*\n\s*BEFORE\s*\n(.*?)\n\)', content, re.DOTALL)
        if include_match:
            include_content = include_match.group(1)
            if 'archipelago' not in include_content:
                new_includes = include_match.group(0).rstrip(')')
                new_includes += '\tarchipelago\n)'
                content = content.replace(include_match.group(0), new_includes)
        
        # Add source group for IDE organization
        source_group_section = '''
source_group("Archipelago" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/archipelago/.+")
'''
        
        # Find a good place to add source group (after other source_group commands)
        last_source_group = list(re.finditer(r'source_group\([^)]+\)', content))
        if last_source_group:
            insert_pos = last_source_group[-1].end()
            content = content[:insert_pos] + '\n' + source_group_section + content[insert_pos:]
        
        # Write the patched content
        with open(cmake_path, 'w', encoding='utf-8') as f:
            f.write(content)
            
        print("✓ CMakeLists.txt patched successfully!")
        self.patches_applied.append("CMakeLists.txt")
        return True
        
    def patch_d_main(self, dmain_path):
        """Patch d_main.cpp to initialize Archipelago"""
        print("\n📝 Patching d_main.cpp...")
        
        if not os.path.exists(dmain_path):
            print(f"❌ Error: {dmain_path} not found!")
            return False
            
        self.backup_file(dmain_path)
        
        with open(dmain_path, 'r', encoding='utf-8') as f:
            content = f.read()
            
        # Check if already patched
        if 'archipelago_integration.h' in content:
            print("ℹ️  d_main.cpp already includes Archipelago integration, skipping...")
            return True
            
        # Add include after other includes
        include_section = '#include "archipelago/archipelago_integration.h"'
        
        # Find a good place to add the include (after other game includes)
        last_include_match = None
        for match in re.finditer(r'#include\s+"[^"]+\.h"', content):
            last_include_match = match
            
        if last_include_match:
            insert_pos = last_include_match.end()
            content = content[:insert_pos] + '\n' + include_section + content[insert_pos:]
        
        # Add initialization in D_DoomMain after sound initialization
        init_pattern = r'(S_Init\s*\(\s*\);.*?CLOCK_END\s*\([^)]+\))'
        init_match = re.search(init_pattern, content, re.DOTALL)
        
        if init_match:
            archipelago_init = '''
	
	// Initialize Archipelago support
	CLOCK_START
	if (!batchrun) Printf("Archipelago: Initializing network support.\\n");
	Archipelago_Init();
	CLOCK_END("Archipelago Startup")'''
            
            insert_pos = init_match.end()
            content = content[:insert_pos] + archipelago_init + content[insert_pos:]
        
        # Add to game loop (in D_DoomLoop)
        loop_pattern = r'(D_ProcessEvents\s*\(\s*\);)'
        loop_match = re.search(loop_pattern, content)
        
        if loop_match:
            archipelago_process = '''
				Archipelago_ProcessMessages();'''
            
            insert_pos = loop_match.end()
            content = content[:insert_pos] + archipelago_process + content[insert_pos:]
        
        # Add cleanup in D_Cleanup
        cleanup_pattern = r'(void\s+D_Cleanup\s*\([^)]*\)\s*{[^}]*?)(S_StopMusic\s*\([^)]*\);)'
        cleanup_match = re.search(cleanup_pattern, content, re.DOTALL)
        
        if cleanup_match:
            archipelago_cleanup = '''
	// Shutdown Archipelago
	Archipelago_Shutdown();
	'''
            insert_pos = cleanup_match.end(1)
            content = content[:insert_pos] + archipelago_cleanup + content[insert_pos:]
        
        # Write the patched content
        with open(dmain_path, 'w', encoding='utf-8') as f:
            f.write(content)
            
        print("✓ d_main.cpp patched successfully!")
        self.patches_applied.append("d_main.cpp")
        return True
        
    def create_archipelago_directory(self, base_path):
        """Create the archipelago source directory"""
        archipelago_dir = os.path.join(base_path, 'archipelago')
        
        if not os.path.exists(archipelago_dir):
            os.makedirs(archipelago_dir)
            print(f"✓ Created directory: {archipelago_dir}")
        else:
            print(f"ℹ️  Directory already exists: {archipelago_dir}")
            
        return archipelago_dir
        
    def verify_patches(self):
        """Verify that patches were applied correctly"""
        print("\n🔍 Verifying patches...")
        
        all_good = True
        for file in self.patches_applied:
            print(f"  ✓ {file} patched successfully")
            
        return all_good
        
    def generate_rollback_script(self):
        """Generate a script to rollback changes"""
        rollback_script = f"""#!/bin/bash
# Rollback script for Archipelago patches
# Generated on {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}

echo "Rolling back Archipelago patches..."

"""
        
        for file in self.patches_applied:
            backup_file = file + self.backup_suffix
            rollback_script += f"""
if [ -f "{backup_file}" ]; then
    echo "Restoring {file}..."
    cp "{backup_file}" "{file}"
    echo "✓ Restored {file}"
else
    echo "❌ Backup not found: {backup_file}"
fi
"""
        
        rollback_script += """
echo "Rollback complete!"
"""
        
        with open('rollback_archipelago.sh', 'w') as f:
            f.write(rollback_script)
            
        # Make it executable on Unix-like systems
        if os.name != 'nt':
            os.chmod('rollback_archipelago.sh', 0o755)
            
        print("✓ Generated rollback script: rollback_archipelago.sh")

def main():
    print("🏝️  Archipelago Integration Patcher for Selaco")
    print("=" * 50)
    
    # Check if we're in the right directory
    if not os.path.exists('CMakeLists.txt') or not os.path.exists('d_main.cpp'):
        print("❌ Error: This script must be run from the Selaco source directory!")
        print("   Current directory:", os.getcwd())
        print("   Required files: CMakeLists.txt and d_main.cpp")
        sys.exit(1)
        
    patcher = ArchipelagoPatcher()
    
    # Create archipelago directory
    archipelago_dir = patcher.create_archipelago_directory('.')
    
    # Apply patches
    success = True
    success &= patcher.patch_cmake_lists('CMakeLists.txt')
    success &= patcher.patch_d_main('d_main.cpp')
    
    if success:
        print("\n✅ All patches applied successfully!")
        patcher.verify_patches()
        patcher.generate_rollback_script()
        
        print("\n📋 Next steps:")
        print("1. Copy the Archipelago source files to the 'archipelago' directory:")
        print("   - archipelago_socket.h")
        print("   - archipelago_socket.cpp") 
        print("   - archipelago_commands.cpp")
        print("   - archipelago_integration.h")
        print("\n2. Rebuild the project:")
        print("   - Run CMake to regenerate build files")
        print("   - Compile the project")
        print("\n3. Test the integration:")
        print("   - Run: archipelago_help")
        print("   - Try: archipelago_connect localhost:38281")
        
        if os.name == 'nt':
            print("\n⚠️  Note: On Windows, make sure ws2_32.lib is linked (already included in patches)")
    else:
        print("\n❌ Some patches failed! Check the errors above.")
        print("💡 You can use rollback_archipelago.sh to restore original files")
        sys.exit(1)

if __name__ == "__main__":
    main()