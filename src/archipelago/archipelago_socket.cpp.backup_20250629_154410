#include "archipelago_socket.h"
#include "c_cvars.h"
#include "doomtype.h"
#include <ixwebsocket/IXNetSystem.h>
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <thread>
#include <cstdarg>

EXTERN_CVAR(Bool, archipelago_debug)

// If Printf isn't available, define a simple version
#ifndef Printf
void Printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
#endif

// If color constants aren't defined
#ifndef TEXTCOLOR_RED
#define TEXTCOLOR_RED ""
#define TEXTCOLOR_GREEN ""
#endif

ArchipelagoSocket::ArchipelagoSocket() 
    : m_connected(false)
    , m_authenticated(false)
    , m_sslFailed(false)
    , m_port(0)
    , m_serverVersionMajor(0)
    , m_serverVersionMinor(5)
    , m_serverVersionBuild(0) {
    
    // Initialize IXWebSocket
    static bool ixInitialized = false;
    if (!ixInitialized) {
        ix::initNetSystem();
        ixInitialized = true;
    }
    
    // Initialize JSON parsers
    m_jsonReaderBuilder["collectComments"] = false;
    m_jsonReaderBuilder["strictRoot"] = false;
    m_jsonReaderBuilder["allowDroppedNullPlaceholders"] = true;
    m_jsonReaderBuilder["allowNumericKeys"] = true;
    
    m_jsonWriterBuilder["indentation"] = "";
    m_jsonWriterBuilder["commentStyle"] = "None";
    
    m_jsonReader.reset(m_jsonReaderBuilder.newCharReader());
    m_jsonWriter.reset(m_jsonWriterBuilder.newStreamWriter());
}

ArchipelagoSocket::~ArchipelagoSocket() {
    Disconnect();
}

bool ArchipelagoSocket::Connect(const std::string& host, uint16_t port, 
                               const std::string& slotName, 
                               const std::string& password) {
    if (m_connected) {
        m_lastError = "Already connected";
        return false;
    }
    
    if (slotName.empty()) {
        m_lastError = "Slot name cannot be empty";
        return false;
    }
    
    m_host = host;
    m_port = port;
    m_slotName = slotName;
    m_password = password;
    m_sslFailed = false;
    
    // Try SSL first
    std::string url = "wss://" + host + ":" + std::to_string(port);
    
    Printf("Connecting to Archipelago server at %s...\n", url.c_str());
    
    // Set up WebSocket callbacks
    m_webSocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        OnMessage(msg);
    });
    
    // Configure WebSocket
    m_webSocket.setUrl(url);
    m_webSocket.enablePerMessageDeflate();
    m_webSocket.setPingInterval(45);
    m_webSocket.disableAutomaticReconnection(); // We'll handle reconnection
    
    // Start connection
    m_webSocket.start();
    
    // Wait for connection
    if (!WaitForConnection()) {
        // If SSL failed, try without SSL
        if (m_sslFailed && url.find("wss://") == 0) {
            Printf("SSL connection failed, trying without SSL...\n");
            m_webSocket.stop();
            
            url = "ws://" + host + ":" + std::to_string(port);
            m_webSocket.setUrl(url);
            m_webSocket.start();
            
            if (!WaitForConnection()) {
                m_lastError = "Failed to connect to server";
                return false;
            }
        } else {
            return false;
        }
    }
    
    // Connected! Now wait for RoomInfo and send handshake
    // The handshake will be sent when we receive RoomInfo
    if (!WaitForAuthentication()) {
        Disconnect();
        return false;
    }
    
    return true;
}

void ArchipelagoSocket::Disconnect() {
    if (!m_connected) {
        return;
    }
    
    m_connected = false;
    m_authenticated = false;
    m_webSocket.stop();
    
    // Clear receive queue
    {
        std::lock_guard<std::mutex> lock(m_recvMutex);
        while (!m_recvQueue.empty()) {
            m_recvQueue.pop();
        }
    }
    
    Printf("Disconnected from Archipelago server\n");
}

void ArchipelagoSocket::OnMessage(const ix::WebSocketMessagePtr& msg) {
    if (msg->type == ix::WebSocketMessageType::Message) {
        if (archipelago_debug) {
            Printf("Received WebSocket message (%zu bytes)\n", msg->str.length());
        }
        ProcessMessage(msg->str);
    } else if (msg->type == ix::WebSocketMessageType::Open) {
        OnOpen();
    } else if (msg->type == ix::WebSocketMessageType::Error) {
        OnError(msg->errorInfo.reason);
    } else if (msg->type == ix::WebSocketMessageType::Close) {
        OnClose();
    }
}

void ArchipelagoSocket::OnOpen() {
    Printf("WebSocket connection established\n");
    m_connected = true;
    // Wait for RoomInfo before sending handshake
}

void ArchipelagoSocket::OnError(const std::string& error) {
    Printf(TEXTCOLOR_RED "WebSocket error: %s\n", error.c_str());
    m_lastError = error;
    
    // Check if this is an SSL error
    if (error.find("SSL") != std::string::npos || 
        error.find("TLS") != std::string::npos ||
        error.find("https") != std::string::npos) {
        m_sslFailed = true;
    }
}

void ArchipelagoSocket::OnClose() {
    Printf("WebSocket connection closed\n");
    m_connected = false;
    m_authenticated = false;
}

bool ArchipelagoSocket::ProcessMessage(const std::string& message) {
    Json::Value root;
    std::string errs;
    std::istringstream stream(message);
    
    if (!Json::parseFromStream(m_jsonReaderBuilder, stream, &root, &errs)) {
        Printf(TEXTCOLOR_RED "Failed to parse JSON message: %s\n", errs.c_str());
        return false;
    }
    
    // Messages are arrays of commands
    if (!root.isArray() || root.empty()) {
        return false;
    }
    
    for (const auto& cmd : root) {
        if (!cmd.isObject() || !cmd.isMember("cmd")) {
            continue;
        }
        
        std::string cmdType = cmd["cmd"].asString();
        
        if (archipelago_debug) {
            Printf("Received command: %s\n", cmdType.c_str());
        }
        
        if (cmdType == "RoomInfo") {
            // Extract server version
            if (cmd.isMember("version")) {
                const auto& version = cmd["version"];
                m_serverVersionMajor = version.get("major", 0).asInt();
                m_serverVersionMinor = version.get("minor", 5).asInt();
                m_serverVersionBuild = version.get("build", 0).asInt();
                Printf("Server version: %d.%d.%d\n", 
                       m_serverVersionMajor, m_serverVersionMinor, m_serverVersionBuild);
            }
            
            // Send handshake after receiving RoomInfo
            SendHandshake();
            
        } else if (cmdType == "Connected") {
            Printf(TEXTCOLOR_GREEN "Successfully connected to Archipelago as '%s'!\n", 
                   m_slotName.c_str());
            
            if (cmd.isMember("slot")) {
                Printf("Slot number: %d\n", cmd["slot"].asInt());
            }
            
            if (cmd.isMember("missing_locations")) {
                Printf("Missing locations: %d\n", static_cast<int>(cmd["missing_locations"].size()));
            }
            
            m_authenticated = true;
            
            // Add to message queue for game processing
            ArchipelagoMessage msg;
            msg.type = ArchipelagoMessageType::CONNECTED;
            msg.json = cmd;
            
            std::ostringstream oss;
            m_jsonWriter->write(cmd, &oss);
            msg.data = oss.str();
            
            std::lock_guard<std::mutex> lock(m_recvMutex);
            m_recvQueue.push(msg);
            
        } else if (cmdType == "ConnectionRefused") {
            std::string reason = "Unknown reason";
            
            if (cmd.isMember("errors") && cmd["errors"].isArray() && !cmd["errors"].empty()) {
                reason = cmd["errors"][0].asString();
            }
            
            m_lastError = "Connection refused: " + reason;
            Printf(TEXTCOLOR_RED "%s\n", m_lastError.c_str());
            m_authenticated = false;
            
        } else if (cmdType == "PrintJSON") {
            // Add to message queue
            ArchipelagoMessage msg;
            msg.type = ArchipelagoMessageType::PRINT_JSON;
            msg.json = cmd;
            
            std::ostringstream oss;
            m_jsonWriter->write(cmd, &oss);
            msg.data = oss.str();
            
            std::lock_guard<std::mutex> lock(m_recvMutex);
            m_recvQueue.push(msg);
            
        } else if (cmdType == "ReceivedItems") {
            // Add to message queue
            ArchipelagoMessage msg;
            msg.type = ArchipelagoMessageType::DATA;
            msg.json = cmd;
            
            std::ostringstream oss;
            m_jsonWriter->write(cmd, &oss);
            msg.data = oss.str();
            
            std::lock_guard<std::mutex> lock(m_recvMutex);
            m_recvQueue.push(msg);
        }
        // Add other message types as needed
    }
    
    return true;
}

bool ArchipelagoSocket::SendHandshake() {
    Json::Value handshake;
    handshake[0]["cmd"] = "Connect";
    handshake[0]["password"] = m_password;
    handshake[0]["name"] = m_slotName;
    
    Json::Value version;
    version["major"] = m_serverVersionMajor > 0 ? m_serverVersionMajor : 0;
    version["minor"] = m_serverVersionMinor;
    version["build"] = m_serverVersionBuild;
    version["class"] = "Version";
    handshake[0]["version"] = version;
    
    handshake[0]["uuid"] = GenerateUUID();
    handshake[0]["game"] = "Selaco";
    handshake[0]["tags"] = Json::arrayValue;
    handshake[0]["slot_data"] = true;
    handshake[0]["items_handling"] = 7; // 0b111 = all items
    
    std::ostringstream oss;
    m_jsonWriter->write(handshake, &oss);
    std::string jsonStr = oss.str();
    
    if (archipelago_debug) {
        Printf("Sending handshake: %s\n", jsonStr.c_str());
    }
    
    return m_webSocket.send(jsonStr).success;
}

bool ArchipelagoSocket::SendJson(const Json::Value& json) {
    if (!m_connected) {
        m_lastError = "Not connected";
        return false;
    }
    
    std::ostringstream oss;
    m_jsonWriter->write(json, &oss);
    return m_webSocket.send(oss.str()).success;
}

bool ArchipelagoSocket::SendMessage(const ArchipelagoMessage& msg) {
    if (!m_connected) {
        m_lastError = "Not connected";
        return false;
    }
    
    Json::Value json;
    
    switch (msg.type) {
        case ArchipelagoMessageType::DATA:
            json[0]["cmd"] = "Say";
            json[0]["text"] = msg.data;
            break;
        default:
            // Use provided JSON if available
            if (!msg.json.empty()) {
                json = msg.json;
            } else {
                json[0]["cmd"] = std::to_string(static_cast<int>(msg.type));
                json[0]["data"] = msg.data;
            }
            break;
    }
    
    return SendJson(json);
}

bool ArchipelagoSocket::ReceiveMessage(ArchipelagoMessage& msg) {
    std::lock_guard<std::mutex> lock(m_recvMutex);
    
    if (m_recvQueue.empty()) {
        return false;
    }
    
    msg = m_recvQueue.front();
    m_recvQueue.pop();
    return true;
}

bool ArchipelagoSocket::HasPendingMessages() const {
    std::lock_guard<std::mutex> lock(m_recvMutex);
    return !m_recvQueue.empty();
}

bool ArchipelagoSocket::WaitForConnection(int timeoutMs) {
    auto start = std::chrono::steady_clock::now();
    
    while (!m_connected && !m_sslFailed) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        
        if (elapsed >= timeoutMs) {
            m_lastError = "Connection timeout";
            return false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return m_connected;
}

bool ArchipelagoSocket::WaitForAuthentication(int timeoutMs) {
    auto start = std::chrono::steady_clock::now();
    
    while (m_connected && !m_authenticated) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        
        if (elapsed >= timeoutMs) {
            m_lastError = "Authentication timeout";
            return false;
        }
        
        if (!m_connected) {
            m_lastError = "Connection lost during authentication";
            return false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return m_authenticated;
}

std::string ArchipelagoSocket::GenerateUUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);
    
    std::stringstream ss;
    ss << "selaco-";
    
    for (int i = 0; i < 8; i++) {
        ss << std::hex << dis(gen);
    }
    ss << "-";
    for (int i = 0; i < 4; i++) {
        ss << std::hex << dis(gen);
    }
    ss << "-4"; // Version 4 UUID
    for (int i = 0; i < 3; i++) {
        ss << std::hex << dis(gen);
    }
    ss << "-";
    ss << std::hex << dis2(gen); // Variant bits
    for (int i = 0; i < 3; i++) {
        ss << std::hex << dis(gen);
    }
    ss << "-";
    for (int i = 0; i < 12; i++) {
        ss << std::hex << dis(gen);
    }
    
    return ss.str();
}

std::string ArchipelagoSocket::GetConnectionInfo() const {
    if (!m_connected) {
        return "Not connected";
    }
    
    std::stringstream info;
    info << "Connected to " << m_host << ":" << m_port;
    if (m_authenticated) {
        info << " (authenticated)";
    } else {
        info << " (not authenticated)";
    }
    return info.str();
}