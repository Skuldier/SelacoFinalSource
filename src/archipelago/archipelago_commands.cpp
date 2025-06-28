#include "archipelago_socket.h"
#include "c_dispatch.h"
#include "c_cvars.h"
#include "doomtype.h"
#include <memory>
#include <sstream>

// Global Archipelago socket instance
static std::unique_ptr<ArchipelagoSocket> g_archipelagoSocket;

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
    }
    
    if (argv.argc() >= 3) {
        // Parse host:port
        std::string hostPort = argv[2];
        size_t colonPos = hostPort.find(':');
        
        if (colonPos != std::string::npos) {
            host = hostPort.substr(0, colonPos);
            port = static_cast<uint16_t>(std::stoi(hostPort.substr(colonPos + 1)));
        } else {
            host = hostPort;
        }
    }
    
    if (argv.argc() >= 4) {
        password = argv[3];
    }
    
    // Validate slot name
    if (slot.empty()) {
        Printf(TEXTCOLOR_RED "Error: Slot name is required!\n");
        Printf("Usage: archipelago_connect <slot_name> [host:port] [password]\n");
        Printf("   or: set archipelago_slot and use archipelago_connect\n");
        return;
    }
    
    Printf("Connecting to %s:%d as '%s'...\n", host.c_str(), port, slot.c_str());
    
    if (g_archipelagoSocket->Connect(host, port, slot, password)) {
        // Save successful connection info
        archipelago_host = host.c_str();
        archipelago_port = port;
        archipelago_slot = slot.c_str();
        archipelago_password = password.c_str();
    } else {
        Printf(TEXTCOLOR_RED "Failed to connect: %s\n", 
               g_archipelagoSocket->GetLastError().c_str());
    }
}

CCMD(archipelago_disconnect) {
    if (!g_archipelagoSocket) {
        Printf("Archipelago system not initialized\n");
        return;
    }
    
    if (!g_archipelagoSocket->IsConnected()) {
        Printf("Not connected to Archipelago server\n");
        return;
    }
    
    g_archipelagoSocket->Disconnect();
    Printf("Disconnected from Archipelago server\n");
}

CCMD(archipelago_status) {
    if (!g_archipelagoSocket) {
        Printf("Archipelago system not initialized\n");
        return;
    }
    
    if (g_archipelagoSocket->IsConnected()) {
        Printf("Status: %s\n", g_archipelagoSocket->GetConnectionInfo().c_str());
        Printf("Slot: %s\n", g_archipelagoSocket->GetSlotName().c_str());
    } else {
        Printf("Status: Not connected\n");
        if (strlen(archipelago_slot) > 0) {
            Printf("Configured slot: %s\n", (const char*)archipelago_slot);
        }
        Printf("Use 'archipelago_connect <slot_name>' to connect\n");
    }
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
    
    ArchipelagoMessage msg;
    msg.type = ArchipelagoMessageType::DATA;
    msg.data = ss.str();
    
    if (g_archipelagoSocket->SendMessage(msg)) {
        Printf("Message sent: %s\n", msg.data.c_str());
    } else {
        Printf(TEXTCOLOR_RED "Failed to send message: %s\n", 
               g_archipelagoSocket->GetLastError().c_str());
    }
}

CCMD(archipelago_debug) {
    archipelago_debug = !archipelago_debug;
    Printf("Archipelago debug mode: %s\n", archipelago_debug ? "ON" : "OFF");
}

// Help command
CCMD(archipelago_help) {
    Printf("Archipelago Commands:\n");
    Printf(TEXTCOLOR_GOLD "  archipelago_connect <slot_name> [host:port] [password]\n");
    Printf("    - Connect to server with specified slot name\n");
    Printf(TEXTCOLOR_GOLD "  archipelago_disconnect\n");
    Printf("    - Disconnect from server\n");
    Printf(TEXTCOLOR_GOLD "  archipelago_status\n");
    Printf("    - Show connection status\n");
    Printf(TEXTCOLOR_GOLD "  archipelago_setslot <slot_name>\n");
    Printf("    - Set default slot name\n");
    Printf(TEXTCOLOR_GOLD "  archipelago_send <message>\n");
    Printf("    - Send a test message\n");
    Printf(TEXTCOLOR_GOLD "  archipelago_debug\n");
    Printf("    - Toggle debug mode\n");
    Printf("\nCVars:\n");
    Printf("  archipelago_host - Server hostname (current: %s)\n", 
           (const char*)archipelago_host);
    Printf("  archipelago_port - Server port (current: %d)\n", 
           (int)archipelago_port);
    Printf("  archipelago_slot - Default slot name (current: %s)\n",
           strlen(archipelago_slot) > 0 ? (const char*)archipelago_slot : "<not set>");
    Printf("  archipelago_password - Default password\n");
    Printf("  archipelago_autoconnect - Auto-connect on startup (current: %s)\n",
           archipelago_autoconnect ? "true" : "false");
}