#!/usr/bin/env python3
"""
Script to fix the Archipelago menu error by removing problematic AddOptionMenu calls
"""

import sys
import re

def fix_menudef(filepath):
    """Remove problematic AddOptionMenu entries that cause errors"""
    
    with open(filepath, 'r') as f:
        content = f.read()
    
    # Pattern to find and remove MultiplayerOptions block
    pattern = r'AddOptionMenu\s+"MultiplayerOptions"\s*\{[^}]*\}'
    
    # Remove the problematic section
    fixed_content = re.sub(pattern, '', content, flags=re.DOTALL)
    
    # Also try to remove NetworkOptions if it causes issues
    # Uncomment the next line if NetworkOptions also causes problems
    # fixed_content = re.sub(r'AddOptionMenu\s+"NetworkOptions"\s*\{[^}]*\}', '', fixed_content, flags=re.DOTALL)
    
    # Write back
    with open(filepath, 'w') as f:
        f.write(fixed_content)
    
    print(f"Fixed {filepath}")
    print("Removed problematic AddOptionMenu entries")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        fix_menudef(sys.argv[1])
    else:
        fix_menudef("wadsrc/static/menudef.txt")