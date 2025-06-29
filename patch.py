#!/usr/bin/env python3
"""
Archipelago Socket Implementation Auto-Patcher
Automatically fixes compilation errors in the Archipelago socket implementation
"""

import os
import re
import shutil
from datetime import datetime
from typing import List, Tuple

class ArchipelagoPatcher:
    def __init__(self, base_path: str = "."):
        self.base_path = base_path
        self.fixes_applied = []
        
    def backup_file(self, filepath: str):
        """Create a backup of the file before patching"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        backup_path = f"{filepath}.backup_{timestamp}"
        shutil.copy2(filepath, backup_path)
        print(f"✓ Created backup: {backup_path}")
        
    def read_file(self, filepath: str) -> str:
        """Read file content"""
        with open(filepath, 'r', encoding='utf-8') as f:
            return f.read()
            
    def write_file(self, filepath: str, content: str):
        """Write content to file"""
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
            
    def patch_archipelago_socket_h(self):
        """Fix archipelago_socket.h"""
        filepath = os.path.join(self.base_path, "src/archipelago/archipelago_socket.h")
        if not os.path.exists(filepath):
            print(f"✗ File not found: {filepath}")
            return False
            
        print(f"\n📝 Patching {filepath}...")
        self.backup_file(filepath)
        
        content = self.read_file(filepath)
        original_content = content
        
        # Fix 1: Update Windows includes section with SendMessage undef
        windows_section_pattern = r'(#ifdef _WIN32\s*\n.*?#pragma comment\(lib, "ws2_32\.lib"\)\s*\n)(.*?typedef SOCKET socket_t;)'
        windows_replacement = r'''\1    
    // Undefine Windows macros that conflict with our method names
    #ifdef SendMessage
        #undef SendMessage
    #endif
    
    \2'''
        content = re.sub(windows_section_pattern, windows_replacement, content, flags=re.DOTALL)
        
        # Fix 2: Update non-Windows section with additional defines
        nonwindows_pattern = r'(#else\s*\n.*?#define WSAEWOULDBLOCK EWOULDBLOCK\s*\n)(#endif)'
        nonwindows_replacement = r'''\1    #define ioctlsocket ioctl
    #define SOCKET int
    #define DWORD unsigned long
    #define WSAETIMEDOUT ETIMEDOUT
\2'''
        content = re.sub(nonwindows_pattern, nonwindows_replacement, content, flags=re.DOTALL)
        
        # Fix 3: Add missing include for non-Windows
        nonwindows_includes = r'(#include <errno\.h>\s*\n)'
        content = re.sub(nonwindows_includes, r'\1    #include <sys/ioctl.h>\n', content)
        
        # Fix 4: Add missing private members and methods
        # Find the private section and add missing members
        private_section_pattern = r'(mutable std::mutex m_recvMutex;\s*\n\s*\n\s*static int s_socketsInitialized;\s*\n)(};)'
        
        missing_members = """    
    // Additional private members
    bool m_authenticated;
    bool m_sslFailed;
    
    // Additional private methods
    void OnMessage(const std::string& message);
    void OnOpen();
    void OnError(const std::string& error);
    void OnClose();
    void ProcessMessage(const std::string& message);
    bool SendJson(const std::string& json);
"""
        
        content = re.sub(private_section_pattern, r'\1' + missing_members + r'\n\2', content)
        
        if content != original_content:
            self.write_file(filepath, content)
            self.fixes_applied.append(f"archipelago_socket.h - Added missing members and Windows fixes")
            print("✓ Fixed archipelago_socket.h")
            return True
        else:
            print("⚠ No changes needed for archipelago_socket.h")
            return False
            
    def patch_archipelago_socket_cpp(self):
        """Fix archipelago_socket.cpp"""
        filepath = os.path.join(self.base_path, "src/archipelago/archipelago_socket.cpp")
        if not os.path.exists(filepath):
            print(f"✗ File not found: {filepath}")
            return False
            
        print(f"\n📝 Patching {filepath}...")
        self.backup_file(filepath)
        
        content = self.read_file(filepath)
        original_content = content
        
        # Fix 1: Remove Printf redefinition (if it exists)
        # Look for void Printf definition and remove it
        printf_pattern = r'void\s+Printf\s*\([^)]*\)\s*{[^}]*}'
        content = re.sub(printf_pattern, '', content)
        
        # Fix 2: Add sys/ioctl.h include for non-Windows
        ifndef_pattern = r'(#ifndef _WIN32\s*\n#include <fcntl\.h>\s*\n)'
        content = re.sub(ifndef_pattern, r'\1#include <sys/ioctl.h>\n', content)
        
        # Fix 3: Fix constructor to initialize new members
        constructor_pattern = r'(ArchipelagoSocket::ArchipelagoSocket\(\)\s*\n\s*:\s*m_socket\(INVALID_SOCKET\)\s*\n\s*,\s*m_connected\(false\)\s*\n\s*,\s*m_port\(0\)\s*\n\s*,\s*m_shouldStop\(false\))\s*{'
        constructor_replacement = r'''\1
    , m_authenticated(false)
    , m_sslFailed(false) {'''
        content = re.sub(constructor_pattern, constructor_replacement, content)
        
        # Fix 4: Add missing method implementations at the end of file
        if 'void ArchipelagoSocket::OnMessage' not in content:
            missing_methods = '''
// Missing method implementations
void ArchipelagoSocket::OnMessage(const std::string& message) {
    if (archipelago_debug) {
        Printf("OnMessage: %s\\n", message.c_str());
    }
    ProcessMessage(message);
}

void ArchipelagoSocket::OnOpen() {
    Printf("WebSocket connection opened\\n");
    m_connected = true;
}

void ArchipelagoSocket::OnError(const std::string& error) {
    Printf("WebSocket error: %s\\n", error.c_str());
    m_lastError = error;
    
    // Check for SSL-specific errors
    if (error.find("SSL") != std::string::npos || 
        error.find("certificate") != std::string::npos) {
        m_sslFailed = true;
    }
}

void ArchipelagoSocket::OnClose() {
    Printf("WebSocket connection closed\\n");
    m_connected = false;
    m_authenticated = false;
}

void ArchipelagoSocket::ProcessMessage(const std::string& message) {
    // This method processes incoming WebSocket messages
    // Parse and handle the message appropriately
    ArchipelagoMessage msg;
    if (ParseJsonMessage(message, msg)) {
        std::lock_guard<std::mutex> lock(m_recvMutex);
        m_recvQueue.push(msg);
    }
}

bool ArchipelagoSocket::SendJson(const std::string& json) {
    return SendWebSocketFrame(json);
}
'''
            # Add before the final closing brace or at the end
            content = content.rstrip() + missing_methods
        
        if content != original_content:
            self.write_file(filepath, content)
            self.fixes_applied.append(f"archipelago_socket.cpp - Removed Printf, added methods")
            print("✓ Fixed archipelago_socket.cpp")
            return True
        else:
            print("⚠ No changes needed for archipelago_socket.cpp")
            return False
            
    def patch_archipelago_commands_cpp(self):
        """Fix archipelago_commands.cpp"""
        filepath = os.path.join(self.base_path, "src/archipelago/archipelago_commands.cpp")
        if not os.path.exists(filepath):
            print(f"✗ File not found: {filepath}")
            return False
            
        print(f"\n📝 Patching {filepath}...")
        self.backup_file(filepath)
        
        content = self.read_file(filepath)
        original_content = content
        
        # Fix 1: Add Windows socket headers after existing includes
        includes_pattern = r'(#include <sstream>\s*\n)'
        socket_includes = r'''\1
// Add socket headers for raw test command
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
    #define WSAETIMEDOUT ETIMEDOUT
    #define DWORD unsigned long
#endif

'''
        content = re.sub(includes_pattern, socket_includes, content)
        
        # Fix 2: Fix the broken Cmd_archipelago_test_raw function
        # First, remove any broken version
        broken_pattern = r'// Console command for raw Archipelago test\s*\nCCMD.*?(?=(?:CCMD|CVAR|$))'
        content = re.sub(broken_pattern, '', content, flags=re.DOTALL)
        
        # Remove any trailing broken code
        broken_end_pattern = r'Printf\("Testing raw Archipelago connection.*?Cmd_archipelago_help.*?$'
        content = re.sub(broken_end_pattern, '', content, flags=re.DOTALL)
        
        # Add the correct implementation at the end
        correct_implementation = '''
// Console command for raw Archipelago test
CCMD(archipelago_test_raw)
{
    Printf("Testing raw Archipelago connection...\\n");
    
    // Test basic socket operations
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        Printf("WSAStartup failed\\n");
        return;
    }
#endif
    
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        Printf("Failed to create socket\\n");
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }
    
    // Set timeout
#ifdef _WIN32
    DWORD timeout = 5000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
    
    const char* host = "localhost";
    int port = 38281;
    
    struct hostent* hostinfo = gethostbyname(host);
    if (!hostinfo) {
        Printf("Failed to resolve hostname\\n");
        closesocket(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *((struct in_addr*)hostinfo->h_addr);
    
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        Printf("Failed to connect\\n");
        closesocket(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }
    
    Printf("Connected! Sending WebSocket upgrade...\\n");
    
    // Send WebSocket upgrade request
    const char* request = 
        "GET / HTTP/1.1\\r\\n"
        "Host: localhost:38281\\r\\n"
        "Upgrade: websocket\\r\\n"
        "Connection: Upgrade\\r\\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\\r\\n"
        "Sec-WebSocket-Version: 13\\r\\n"
        "\\r\\n";
    
    if (send(sock, request, strlen(request), 0) == SOCKET_ERROR) {
        Printf("Failed to send upgrade request\\n");
        closesocket(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }
    
    // Read response
    char buffer[1024];
    int received = recv(sock, buffer, sizeof(buffer)-1, 0);
    if (received > 0) {
        buffer[received] = '\\0';
        Printf("Received response:\\n%s\\n", buffer);
        
        if (strstr(buffer, "101 Switching Protocols")) {
            Printf("WebSocket upgrade successful!\\n");
        } else {
            Printf("WebSocket upgrade failed\\n");
        }
    } else {
        if (received == 0) {
            Printf("Connection closed by server\\n");
        } else {
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error == WSAETIMEDOUT) {
                Printf("Connection timed out\\n");
            } else {
                Printf("Receive failed with error: %d\\n", error);
            }
#else
            Printf("Receive failed\\n");
#endif
        }
    }
    
    closesocket(sock);
    
#ifdef _WIN32
    WSACleanup();
#endif
    
    Printf("Test complete\\n");
}

// Console command for help
CCMD(archipelago_help)
{
    Printf("Archipelago commands:\\n");
    Printf("  archipelago_connect - Connect to Archipelago server\\n");
    Printf("  archipelago_disconnect - Disconnect from server\\n");
    Printf("  archipelago_status - Show connection status\\n");
    Printf("  archipelago_send <message> - Send a message\\n");
    Printf("  archipelago_test_raw - Test raw socket connection\\n");
    Printf("\\nCVars:\\n");
    Printf("  archipelago_host - Server hostname (default: localhost)\\n");
    Printf("  archipelago_port - Server port (default: 38281)\\n");
    Printf("  archipelago_slot - Slot name\\n");
    Printf("  archipelago_password - Password (optional)\\n");
    Printf("  archipelago_autoconnect - Auto-connect on startup\\n");
    Printf("  archipelago_debug - Enable debug messages\\n");
}
'''
        
        # Ensure content ends properly and add the implementations
        content = content.rstrip() + '\n' + correct_implementation
        
        if content != original_content:
            self.write_file(filepath, content)
            self.fixes_applied.append(f"archipelago_commands.cpp - Added socket headers and fixed commands")
            print("✓ Fixed archipelago_commands.cpp")
            return True
        else:
            print("⚠ No changes needed for archipelago_commands.cpp")
            return False
            
    def run(self):
        """Run all patches"""
        print("🚀 Archipelago Socket Auto-Patcher")
        print("=" * 50)
        
        # Check if we're in the right directory
        if not os.path.exists(os.path.join(self.base_path, "src/archipelago")):
            print("❌ Error: Cannot find src/archipelago directory!")
            print("   Please run this script from the SelacoFinalSource root directory")
            return False
            
        success = True
        
        # Apply all patches
        success &= self.patch_archipelago_socket_h()
        success &= self.patch_archipelago_socket_cpp()
        success &= self.patch_archipelago_commands_cpp()
        
        print("\n" + "=" * 50)
        if self.fixes_applied:
            print("✅ Patching complete! Fixes applied:")
            for fix in self.fixes_applied:
                print(f"   • {fix}")
        else:
            print("⚠️  No fixes were needed - files may already be patched")
            
        print("\n💡 Next steps:")
        print("   1. Review the changes (backups were created)")
        print("   2. Rebuild the project")
        print("   3. If build still fails, check the error messages")
        
        return success

def main():
    import sys
    
    # Allow specifying base path as argument
    base_path = sys.argv[1] if len(sys.argv) > 1 else "."
    
    patcher = ArchipelagoPatcher(base_path)
    success = patcher.run()
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()