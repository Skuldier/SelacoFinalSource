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
CVAR(Bool, archipelago_autoconnect, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
CVAR(Bool, archipelago_debug, false, CVAR_ARCHIVE)

// Initialize Archipelago system
void Archipelago_Init() {
    if (!g_archipelagoSocket) {
        g_archipelagoSocket = std::make_unique<ArchipelagoSocket>();
        
        if (archipelago_autoconnect) {
            const char* hostStr = archipelago_host;
            g_archipelagoSocket->Connect(std::string(hostStr), archipelago_port);
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
            case ArchipelagoMessageType::DATA:
                // TODO: Process game data
                Printf("Archipelago: Data received: %s\n", msg.data.c_str());
                break;
                
            case ArchipelagoMessageType::MSG_ERROR:
                Printf(TEXTCOLOR_RED "Archipelago error: %s\n", msg.data.c_str());
                break;
                
            default:
                break;
        }
    }
}

// Console Commands

CCMD(archipelago_connect) {
    if (argv.argc() <= 1) {
        // Use CVars
        if (!g_archipelagoSocket) {
            g_archipelagoSocket = std::make_unique<ArchipelagoSocket>();
        }
        
        if (g_archipelagoSocket->IsConnected()) {
            Printf("Already connected to Archipelago server\n");
            return;
        }
        
        const char* hostStr = archipelago_host;
        if (g_archipelagoSocket->Connect(std::string(hostStr), archipelago_port)) {
            Printf("Connected to Archipelago server\n");
        } else {
            Printf(TEXTCOLOR_RED "Failed to connect: %s\n", 
                   g_archipelagoSocket->GetLastError().c_str());
        }
    } else if (argv.argc() == 2) {
        // Parse host:port
        std::string hostPort = argv[1];
        size_t colonPos = hostPort.find(':');
        
        std::string host;
        uint16_t port = 38281; // Default Archipelago port
        
        if (colonPos != std::string::npos) {
            host = hostPort.substr(0, colonPos);
            port = static_cast<uint16_t>(std::stoi(hostPort.substr(colonPos + 1)));
        } else {
            host = hostPort;
        }
        
        if (!g_archipelagoSocket) {
            g_archipelagoSocket = std::make_unique<ArchipelagoSocket>();
        }
        
        if (g_archipelagoSocket->IsConnected()) {
            Printf("Already connected to Archipelago server\n");
            return;
        }
        
        if (g_archipelagoSocket->Connect(host, port)) {
            Printf("Connected to Archipelago server at %s:%d\n", host.c_str(), port);
        } else {
            Printf(TEXTCOLOR_RED "Failed to connect: %s\n", 
                   g_archipelagoSocket->GetLastError().c_str());
        }
    } else {
        Printf("Usage: archipelago_connect [host:port]\n");
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
    } else {
        Printf("Status: Not connected\n");
        Printf("Use 'archipelago_connect' to connect to a server\n");
    }
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
    Printf("  archipelago_connect [host:port] - Connect to Archipelago server\n");
    Printf("  archipelago_disconnect - Disconnect from server\n");
    Printf("  archipelago_status - Show connection status\n");
    Printf("  archipelago_send <message> - Send a test message\n");
    Printf("  archipelago_debug - Toggle debug mode\n");
    Printf("\nCVars:\n");
    Printf("  archipelago_host - Default server hostname (current: %s)\n", 
           (const char*)archipelago_host);
    Printf("  archipelago_port - Default server port (current: %d)\n", 
           (int)archipelago_port);
    Printf("  archipelago_autoconnect - Auto-connect on startup (current: %s)\n",
           archipelago_autoconnect ? "true" : "false");
}