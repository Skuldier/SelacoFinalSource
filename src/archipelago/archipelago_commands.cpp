#include "archipelago_socket.h"
#include "c_dispatch.h"
#include "c_cvars.h"
#include "doomtype.h"
#include <memory>
#include <sstream>

// Global Archipelago socket instance
std::unique_ptr<ArchipelagoSocket> g_archipelagoSocket;

// CVars for Archipelago settings
CVAR(String, archipelago_host, "localhost", CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CVAR(Int, archipelago_port, 38281, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CVAR(String, archipelago_slot, "", CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CVAR(String, archipelago_password, "", CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CVAR(Bool, archipelago_autoconnect, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CVAR(Bool, archipelago_debug, false, CVAR_ARCHIVE)

// Initialize Archipelago system
void Archipelago_Init() {
    if (!g_archipelagoSocket) {
        g_archipelagoSocket = std::make_unique<ArchipelagoSocket>();
        
        if (archipelago_autoconnect && strlen(archipelago_slot) > 0) {
            const char* hostStr = archipelago_host;
            const char* slotStr = archipelago_slot;
            const char* passStr = archipelago_password;
            
            g_archipelagoSocket->Connect(
                std::string(hostStr), 
                archipelago_port,
                std::string(slotStr),
                std::string(passStr)
            );
        }
    }
}

// Shutdown Archipelago system
void Archipelago_Shutdown() {
    if (g_archipelagoSocket) {
        g_archipelagoSocket->Disconnect();
        g_archipelagoSocket.reset();
    }
}

// Process incoming messages (call from main game loop)
void Archipelago_ProcessMessages() {
    if (!g_archipelagoSocket || !g_archipelagoSocket->IsConnected()) {
        return;
    }
    
    ArchipelagoMessage msg;
    while (g_archipelagoSocket->ReceiveMessage(msg)) {
        if (archipelago_debug) {
            Printf("Archipelago: Received message type %d, size %zu\n", 
                   static_cast<int>(msg.type), msg.data.size());
        }
        
        // Handle different message types
        switch (msg.type) {
            case ArchipelagoMessageType::CONNECTED:
                Printf(TEXTCOLOR_GREEN "Archipelago: Successfully connected as '%s'\n", 
                       g_archipelagoSocket->GetSlotName().c_str());
                break;
                
            case ArchipelagoMessageType::REJECTED:
                Printf(TEXTCOLOR_RED "Archipelago: Connection rejected: %s\n", msg.data.c_str());
                g_archipelagoSocket->Disconnect();
                break;
                
            case ArchipelagoMessageType::DATA:
                Printf("Archipelago: Data received: %s\n", msg.data.c_str());
                break;
                
            case ArchipelagoMessageType::PRINT:
            case ArchipelagoMessageType::PRINT_JSON:
                Printf("Archipelago: %s\n", msg.data.c_str());
                break;
                
            case ArchipelagoMessageType::MSG_ERROR:
                Printf(TEXTCOLOR_RED "Archipelago error: %s\n", msg.data.c_str());
                break;
                
            default:
                if (archipelago_debug) {
                    Printf("Archipelago: Unhandled message type %d\n", static_cast<int>(msg.type));
                }
                break;
        }
    }
}

// Console Commands

CCMD(archipelago_connect) {
    if (!g_archipelagoSocket) {
        g_archipelagoSocket = std::make_unique<ArchipelagoSocket>();
    }
    
    if (g_archipelagoSocket->IsConnected()) {
        Printf("Already connected to Archipelago server\n");
        return;
    }
    
    // Convert CVars to std::string properly
    const char* hostCStr = archipelago_host;
    const char* slotCStr = archipelago_slot;
    const char* passCStr = archipelago_password;
    
    std::string host = hostCStr ? hostCStr : "localhost";
    uint16_t port = archipelago_port;
    std::string slot = slotCStr ? slotCStr : "";
    std::string password = passCStr ? passCStr : "";
    
    // Parse arguments
    if (argv.argc() >= 2) {
        slot = argv[1];
    } else if (slot.empty()) {
        Printf("Usage: archipelago_connect <slot_name> [host:port] [password]\n");
        Printf("  or set archipelago_slot CVAR and use: archipelago_connect\n");
        return;
    }
    
    // Parse host:port if provided
    if (argv.argc() >= 3) {
        std::string hostPort = argv[2];
        size_t colonPos = hostPort.find(':');
        if (colonPos != std::string::npos) {
            host = hostPort.substr(0, colonPos);
            port = static_cast<uint16_t>(atoi(hostPort.substr(colonPos + 1).c_str()));
        } else {
            host = hostPort;
        }
    }
    
    // Parse password if provided
    if (argv.argc() >= 4) {
        password = argv[3];
    }
    
    Printf("Connecting to %s:%d as '%s'...\n", host.c_str(), port, slot.c_str());
    
    if (g_archipelagoSocket->Connect(host, port, slot, password)) {
        Printf(TEXTCOLOR_GREEN "Connection initiated\n");
    } else {
        Printf(TEXTCOLOR_RED "Connection failed: %s\n", 
               g_archipelagoSocket->GetLastError().c_str());
    }
}

CCMD(archipelago_disconnect) {
    if (!g_archipelagoSocket || !g_archipelagoSocket->IsConnected()) {
        Printf("Not connected to Archipelago server\n");
        return;
    }
    
    g_archipelagoSocket->Disconnect();
    Printf("Disconnected from Archipelago server\n");
}

CCMD(archipelago_status) {
    if (!g_archipelagoSocket) {
        Printf("Archipelago socket not initialized\n");
        return;
    }
    
    if (g_archipelagoSocket->IsConnected()) {
        Printf(TEXTCOLOR_GREEN "Status: Connected\n");
        Printf("  %s\n", g_archipelagoSocket->GetConnectionInfo().c_str());
    } else {
        Printf(TEXTCOLOR_ORANGE "Status: Not connected\n");
        Printf("  Default server: %s:%d\n", (const char*)archipelago_host, (int)archipelago_port);
        Printf("  Default slot: %s\n", 
               strlen(archipelago_slot) > 0 ? (const char*)archipelago_slot : "<not set>");
    }
    
    Printf("\nSettings:\n");
    Printf("  Auto-connect: %s\n", archipelago_autoconnect ? "enabled" : "disabled");
    Printf("  Debug mode: %s\n", archipelago_debug ? "enabled" : "disabled");
}

CCMD(archipelago_setslot) {
    if (argv.argc() < 2) {
        Printf("Current slot: %s\n", 
               strlen(archipelago_slot) > 0 ? (const char*)archipelago_slot : "<not set>");
        Printf("Usage: archipelago_setslot <slot_name>\n");
        return;
    }
    
    archipelago_slot = argv[1];
    Printf("Slot name set to: %s\n", argv[1]);
}

CCMD(archipelago_send) {
    if (argv.argc() < 2) {
        Printf("Usage: archipelago_send <message>\n");
        return;
    }
    
    if (!g_archipelagoSocket || !g_archipelagoSocket->IsConnected()) {
        Printf("Not connected to Archipelago server\n");
        return;
    }
    
    // Combine all arguments into one message
    std::stringstream ss;
    for (int i = 1; i < argv.argc(); i++) {
        if (i > 1) ss << " ";
        ss << argv[i];
    }
    
    // Create Say command
    std::stringstream json;
    json << "[{\"cmd\":\"Say\",\"text\":\"" << ss.str() << "\"}]";
    
    ArchipelagoMessage msg;
    msg.type = ArchipelagoMessageType::DATA;
    msg.data = json.str();
    
    if (g_archipelagoSocket->SendMessage(msg)) {
        Printf("Message sent: %s\n", ss.str().c_str());
    } else {
        Printf(TEXTCOLOR_RED "Failed to send message: %s\n", 
               g_archipelagoSocket->GetLastError().c_str());
    }
}

CCMD(archipelago_debug) {
    archipelago_debug = !archipelago_debug;
    Printf("Archipelago debug mode: %s\n", archipelago_debug ? "ON" : "OFF");
}

// Test raw WebSocket connection
CCMD(archipelago_test_raw) {
    const char* host = archipelago_host;
    int port = archipelago_port;
    
    if (argv.argc() >= 2) {
        // Parse host:port from argument
        std::string hostPort = argv[1];
        size_t colonPos = hostPort.find(':');
        
        if (colonPos != std::string::npos) {
            host = hostPort.substr(0, colonPos).c_str();
            port = atoi(hostPort.substr(colonPos + 1).c_str());
        } else {
            host = hostPort.c_str();
        }
    }
    
    Printf("=== Archipelago Raw Connection Test ===\n");
    Printf("Testing: %s:%d\n\n", host, port);
    
    // Initialize sockets if needed
    #ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    #endif
    
    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        Printf(TEXTCOLOR_RED "Failed to create socket\n");
        return;
    }
    
    // Set timeout
    #ifdef _WIN32
        DWORD timeout = 5000; // 5 seconds
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    #else
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    #endif
    
    // Resolve host
    Printf("1. Resolving hostname...\n");
    struct hostent* hostinfo = gethostbyname(host);
    if (!hostinfo) {
        Printf(TEXTCOLOR_RED "   Failed to resolve %s\n", host);
        closesocket(sock);
        return;
    }
    Printf(TEXTCOLOR_GREEN "   ✓ Resolved successfully\n");
    
    // Connect
    Printf("\n2. Connecting TCP socket...\n");
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *((struct in_addr*)hostinfo->h_addr);
    
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
        Printf(TEXTCOLOR_RED "   Failed to connect\n");
        closesocket(sock);
        return;
    }
    Printf(TEXTCOLOR_GREEN "   ✓ TCP connected\n");
    
    // Send WebSocket handshake
    Printf("\n3. Sending WebSocket handshake...\n");
    std::stringstream request;
    request << "GET / HTTP/1.1\r\n";
    request << "Host: " << host << ":" << port << "\r\n";
    request << "Upgrade: websocket\r\n";
    request << "Connection: Upgrade\r\n";
    request << "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n";
    request << "Sec-WebSocket-Version: 13\r\n";
    request << "\r\n";
    
    std::string req = request.str();
    Printf("   Request (%zu bytes):\n", req.length());
    Printf(TEXTCOLOR_CYAN "%s", req.c_str());
    
    int sent = send(sock, req.c_str(), req.length(), 0);
    if (sent != req.length()) {
        Printf(TEXTCOLOR_RED "   Failed to send complete request\n");
        closesocket(sock);
        return;
    }
    Printf(TEXTCOLOR_GREEN "   ✓ Sent %d bytes\n", sent);
    
    // Receive response
    Printf("\n4. Waiting for response...\n");
    char buffer[4096];
    int totalReceived = 0;
    std::string response;
    
    // Try to receive with a timeout
    while (totalReceived < sizeof(buffer) - 1) {
        int bytes = recv(sock, buffer + totalReceived, sizeof(buffer) - totalReceived - 1, 0);
        
        if (bytes > 0) {
            totalReceived += bytes;
            buffer[totalReceived] = '\0';
            
            // Check if we have complete headers
            if (strstr(buffer, "\r\n\r\n") != nullptr) {
                Printf(TEXTCOLOR_GREEN "   ✓ Received complete headers (%d bytes)\n", totalReceived);
                break;
            }
        } else if (bytes == 0) {
            Printf(TEXTCOLOR_YELLOW "   Connection closed by server\n");
            break;
        } else {
            #ifdef _WIN32
                int error = WSAGetLastError();
                if (error == WSAETIMEDOUT) {
                    Printf(TEXTCOLOR_YELLOW "   Timeout after %d bytes\n", totalReceived);
                } else {
                    Printf(TEXTCOLOR_RED "   Socket error: %d\n", error);
                }
            #else
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    Printf(TEXTCOLOR_YELLOW "   Timeout after %d bytes\n", totalReceived);
                } else {
                    Printf(TEXTCOLOR_RED "   Socket error: %s\n", strerror(errno));
                }
            #endif
            break;
        }
    }
    
    closesocket(sock);
    
    // Display response
    Printf("\n5. Response analysis:\n");
    if (totalReceived > 0) {
        buffer[totalReceived] = '\0';
        
        // Show first line
        char* firstLine = buffer;
        char* lineEnd = strstr(buffer, "\r\n");
        if (lineEnd) {
            *lineEnd = '\0';
            Printf("   Status: %s%s\n", 
                   strstr(firstLine, "101") ? TEXTCOLOR_GREEN : TEXTCOLOR_RED,
                   firstLine);
            *lineEnd = '\r';
        }
        
        // Check for WebSocket headers
        bool hasUpgrade = strstr(buffer, "Upgrade: websocket") || strstr(buffer, "upgrade: websocket");
        bool hasConnection = strstr(buffer, "Connection: Upgrade") || strstr(buffer, "connection: upgrade");
        
        Printf("   Upgrade header: %s%s\n", 
               hasUpgrade ? TEXTCOLOR_GREEN : TEXTCOLOR_RED,
               hasUpgrade ? "Found" : "Missing");
        Printf("   Connection header: %s%s\n",
               hasConnection ? TEXTCOLOR_GREEN : TEXTCOLOR_RED,
               hasConnection ? "Found" : "Missing");
        
        // Show full response
        Printf("\n   Full response:\n");
        Printf(TEXTCOLOR_GRAY "---START---\n%s\n---END---\n", buffer);
        
        // Diagnose common issues
        if (!strstr(buffer, "101")) {
            Printf(TEXTCOLOR_YELLOW "\n⚠ Server did not return 101 Switching Protocols\n");
            Printf("  This might not be a WebSocket endpoint\n");
        }
        if (!hasUpgrade || !hasConnection) {
            Printf(TEXTCOLOR_YELLOW "\n⚠ Missing required WebSocket headers\n");
        }
    } else {
        Printf(TEXTCOLOR_RED "   No response received\n");
        Printf("\nPossible causes:\n");
        Printf("- Port %d is not a WebSocket server\n", port);
        Printf("- Server expects HTTPS/WSS instead of HTTP/WS\n");
        Printf("- Firewall blocking response\n");
        Printf("- Server crashed or closed connection\n");
    }
    
    Printf("\n=== Test Complete ===\n");
}

// Simple version that uses current settings
CCMD(archipelago_test) {
    if (strlen(archipelago_host) == 0) {
        Printf("Please set archipelago_host first\n");
        return;
    }
    
    Printf("Testing connection to %s:%d...\n", 
           (const char*)archipelago_host, (int)archipelago_port);
    
    // Build command with current host:port
    std::stringstream cmd;
    cmd << "archipelago_test_raw " << archipelago_host << ":" << archipelago_port;
    C_DoCommand(cmd.str().c_str());
}

// Help command
CCMD(archipelago_help) {
    Printf("=== Archipelago Commands ===\n");
    Printf(TEXTCOLOR_GOLD "  archipelago_connect <slot_name> [host:port] [password]\n");
    Printf("    Connect to server with specified slot name\n");
    Printf(TEXTCOLOR_GOLD "  archipelago_disconnect\n");
    Printf("    Disconnect from server\n");
    Printf(TEXTCOLOR_GOLD "  archipelago_status\n");
    Printf("    Show connection status\n");
    Printf(TEXTCOLOR_GOLD "  archipelago_setslot <slot_name>\n");
    Printf("    Set default slot name\n");
    Printf(TEXTCOLOR_GOLD "  archipelago_send <message>\n");
    Printf("    Send a chat message\n");
    Printf(TEXTCOLOR_GOLD "  archipelago_debug\n");
    Printf("    Toggle debug mode\n");
    Printf(TEXTCOLOR_GOLD "  archipelago_test\n");
    Printf("    Test connection to current host:port\n");
    Printf(TEXTCOLOR_GOLD "  archipelago_test_raw [host:port]\n");
    Printf("    Raw WebSocket connection test\n");
    Printf("\n=== CVars ===\n");
    Printf("  archipelago_host - Server hostname (current: %s)\n", 
           (const char*)archipelago_host);
    Printf("  archipelago_port - Server port (current: %d)\n", 
           (int)archipelago_port);
    Printf("  archipelago_slot - Default slot name (current: %s)\n",
           strlen(archipelago_slot) > 0 ? (const char*)archipelago_slot : "<not set>");
    Printf("  archipelago_password - Default password\n");
    Printf("  archipelago_autoconnect - Auto-connect on startup (current: %s)\n",
           archipelago_autoconnect ? "true" : "false");
    Printf("\n=== Troubleshooting ===\n");
    Printf("1. Enable debug: archipelago_debug 1\n");
    Printf("2. Test raw connection: archipelago_test_raw host:port\n");
    Printf("3. Check firewall/antivirus settings\n");
    Printf("4. Verify server is running and accepting connections\n");
}