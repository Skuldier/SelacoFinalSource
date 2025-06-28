#include "archipelago_socket.h"
#include "doomtype.h"
#include "c_dispatch.h"
#include "c_cvars.h"
#include <cstring>
#include <sstream>
#include <chrono>
#include <ctime>
#include <random>
#include <iomanip>
#include <algorithm>

#ifndef _WIN32
#include <fcntl.h>
#endif

// External CVAR declaration
EXTERN_CVAR(Bool, archipelago_debug)

// Increased limits for Archipelago
const size_t MAX_WEBSOCKET_PAYLOAD = 10 * 1024 * 1024;  // 10MB max
const size_t RECV_BUFFER_SIZE = 8192;  // 8KB chunks

int ArchipelagoSocket::s_socketsInitialized = 0;

ArchipelagoSocket::ArchipelagoSocket() 
    : m_socket(INVALID_SOCKET)
    , m_connected(false)
    , m_port(0)
    , m_shouldStop(false) {
    
    InitializeSockets();
}

ArchipelagoSocket::~ArchipelagoSocket() {
    Disconnect();
    CleanupSockets();
}

bool ArchipelagoSocket::InitializeSockets() {
    if (s_socketsInitialized++ > 0) {
        return true;
    }

#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        Printf(TEXTCOLOR_RED "WSAStartup failed: %d\n", result);
        return false;
    }
#endif
    return true;
}

void ArchipelagoSocket::CleanupSockets() {
    if (--s_socketsInitialized == 0) {
#ifdef _WIN32
        WSACleanup();
#endif
    }
}

std::string ArchipelagoSocket::GenerateWebSocketKey() {
    static const char* base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 63);
    
    std::string key;
    for (int i = 0; i < 22; ++i) {
        key += base64_chars[dis(gen)];
    }
    return key + "==";
}

uint32_t ArchipelagoSocket::GenerateMaskingKey() {
    std::random_device rd;
    return rd();
}

std::string ArchipelagoSocket::GenerateUUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);
    
    std::stringstream ss;
    int i;
    for (i = 0; i < 8; i++) {
        ss << std::hex << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 4; i++) {
        ss << std::hex << dis(gen);
    }
    ss << "-4";
    for (i = 0; i < 3; i++) {
        ss << std::hex << dis(gen);
    }
    ss << "-";
    ss << std::hex << dis2(gen);
    for (i = 0; i < 3; i++) {
        ss << std::hex << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 12; i++) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
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

    // Create socket
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        m_lastError = "Failed to create socket";
        return false;
    }

    // Resolve hostname
    struct hostent* hostinfo = gethostbyname(host.c_str());
    if (!hostinfo) {
        m_lastError = "Failed to resolve hostname: " + host;
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    // Setup address
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *((struct in_addr*)hostinfo->h_addr);

    // Connect TCP socket
    Printf("Connecting to Archipelago server at %s:%d...\n", host.c_str(), port);
    
    if (connect(m_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        m_lastError = "Failed to connect to server";
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    Printf("TCP connection established, upgrading to WebSocket...\n");
    
    // Perform WebSocket handshake
    if (!PerformWebSocketHandshake()) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    // Store connection info
    m_host = host;
    m_port = port;
    m_slotName = slotName;
    m_password = password;

    // Send Archipelago handshake
    if (!SendHandshake()) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    // Wait for handshake response
    if (!ProcessHandshakeResponse()) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    // Connection successful
    m_connected = true;
    m_shouldStop = false;

    // Start receiver thread
    m_recvThread = std::thread(&ArchipelagoSocket::ReceiverThreadFunc, this);

    return true;
}

void ArchipelagoSocket::Disconnect() {
    if (!m_connected) {
        return;
    }

    m_connected = false;
    m_shouldStop = true;

    // Close socket
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }

    // Wait for receiver thread
    if (m_recvThread.joinable()) {
        m_recvThread.join();
    }

    // Clear receive queue
    {
        std::lock_guard<std::mutex> lock(m_recvMutex);
        while (!m_recvQueue.empty()) {
            m_recvQueue.pop();
        }
    }

    Printf("Disconnected from Archipelago server\n");
}

bool ArchipelagoSocket::PerformWebSocketHandshake() {
    std::string wsKey = GenerateWebSocketKey();
    
    // Build HTTP upgrade request
    std::stringstream request;
    request << "GET / HTTP/1.1\r\n";
    request << "Host: " << m_host << ":" << m_port << "\r\n";
    request << "Upgrade: websocket\r\n";
    request << "Connection: Upgrade\r\n";
    request << "Sec-WebSocket-Key: " << wsKey << "\r\n";
    request << "Sec-WebSocket-Version: 13\r\n";
    request << "\r\n";
    
    std::string requestStr = request.str();
    
    if (archipelago_debug) {
        Printf("Sending WebSocket upgrade request\n");
    }
    
    // Send upgrade request
    if (!SendRawData(requestStr.c_str(), requestStr.length())) {
        return false;
    }
    
    // Read response with dynamic buffer
    std::vector<char> buffer(RECV_BUFFER_SIZE);
    std::string response;
    
    // Read until we have the complete HTTP headers
    while (response.find("\r\n\r\n") == std::string::npos) {
        int bytes = recv(m_socket, buffer.data(), buffer.size() - 1, 0);
        if (bytes <= 0) {
            m_lastError = "Failed to receive WebSocket upgrade response";
            return false;
        }
        response.append(buffer.data(), bytes);
    }
    
    if (archipelago_debug) {
        Printf("WebSocket upgrade response received\n");
    }
    
    // Check for successful upgrade
    if (response.find("HTTP/1.1 101") == std::string::npos) {
        m_lastError = "WebSocket upgrade failed";
        return false;
    }
    
    Printf(TEXTCOLOR_GREEN "WebSocket connection established!\n");
    return true;
}

bool ArchipelagoSocket::SendWebSocketFrame(const std::string& message) {
    std::vector<uint8_t> frame;
    
    // FIN = 1, RSV = 0, Opcode = TEXT (0x1)
    frame.push_back(0x81);
    
    size_t len = message.length();
    
    // Payload length with mask bit set
    if (len < 126) {
        frame.push_back(0x80 | static_cast<uint8_t>(len));
    } else if (len < 65536) {
        frame.push_back(0x80 | 126);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    } else {
        // Support 64-bit length
        frame.push_back(0x80 | 127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back((len >> (i * 8)) & 0xFF);
        }
    }
    
    // Masking key (required for client->server)
    uint32_t maskKey = GenerateMaskingKey();
    uint8_t mask[4];
    mask[0] = (maskKey >> 24) & 0xFF;
    mask[1] = (maskKey >> 16) & 0xFF;
    mask[2] = (maskKey >> 8) & 0xFF;
    mask[3] = maskKey & 0xFF;
    
    for (int i = 0; i < 4; ++i) {
        frame.push_back(mask[i]);
    }
    
    // Masked payload
    for (size_t i = 0; i < message.length(); ++i) {
        frame.push_back(message[i] ^ mask[i % 4]);
    }
    
    // Send frame
    return SendRawData(frame.data(), frame.size());
}

bool ArchipelagoSocket::ReceiveWebSocketFrame(std::string& message) {
    message.clear();
    
    // Support for fragmented frames
    bool finalFrame = false;
    uint8_t firstOpcode = 0;
    
    while (!finalFrame) {
        uint8_t header[2];
        
        // Read frame header
        if (!ReceiveRawData(header, 2)) {
            return false;
        }
        
        finalFrame = (header[0] & 0x80) != 0;
        uint8_t opcode = header[0] & 0x0F;
        bool masked = (header[1] & 0x80) != 0;
        
        // Store first frame's opcode
        if (firstOpcode == 0) {
            firstOpcode = opcode;
        }
        
        // Get payload length
        uint64_t payloadLen = header[1] & 0x7F;
        
        // Extended payload length
        if (payloadLen == 126) {
            uint8_t extLen[2];
            if (!ReceiveRawData(extLen, 2)) {
                return false;
            }
            payloadLen = (static_cast<uint64_t>(extLen[0]) << 8) | extLen[1];
        } else if (payloadLen == 127) {
            // Support 64-bit length
            uint8_t extLen[8];
            if (!ReceiveRawData(extLen, 8)) {
                return false;
            }
            payloadLen = 0;
            for (int i = 0; i < 8; ++i) {
                payloadLen = (payloadLen << 8) | extLen[i];
            }
            
            // Sanity check
            if (payloadLen > MAX_WEBSOCKET_PAYLOAD) {
                m_lastError = "WebSocket frame too large";
                return false;
            }
        }
        
        // Masking key (if present)
        uint8_t mask[4] = {0};
        if (masked) {
            if (!ReceiveRawData(mask, 4)) {
                return false;
            }
        }
        
        // Read payload in chunks for large messages
        std::vector<uint8_t> payload(payloadLen);
        size_t totalRead = 0;
        
        while (totalRead < payloadLen) {
            size_t toRead = std::min(RECV_BUFFER_SIZE, payloadLen - totalRead);
            if (!ReceiveRawData(payload.data() + totalRead, toRead)) {
                return false;
            }
            totalRead += toRead;
        }
        
        // Unmask payload if needed
        if (masked) {
            for (size_t i = 0; i < payload.size(); ++i) {
                payload[i] ^= mask[i % 4];
            }
        }
        
        // Handle different frame types
        if (opcode == 0x0) { // Continuation frame
            message.append(payload.begin(), payload.end());
        } else if (opcode == 0x1) { // Text frame
            message.append(payload.begin(), payload.end());
        } else if (opcode == 0x8) { // Close frame
            m_connected = false;
            return false;
        } else if (opcode == 0x9) { // Ping frame
            // Send pong response
            SendPongFrame(payload);
            continue; // Don't return, wait for next frame
        } else if (opcode == 0xA) { // Pong frame
            continue; // Ignore pongs
        }
    }
    
    return !message.empty();
}

bool ArchipelagoSocket::SendPongFrame(const std::vector<uint8_t>& pingData) {
    std::vector<uint8_t> pongFrame;
    pongFrame.push_back(0x8A); // FIN + PONG opcode
    
    if (pingData.size() < 126) {
        pongFrame.push_back(0x80 | static_cast<uint8_t>(pingData.size()));
    } else {
        pongFrame.push_back(0x80 | 126);
        pongFrame.push_back((pingData.size() >> 8) & 0xFF);
        pongFrame.push_back(pingData.size() & 0xFF);
    }
    
    // Add mask key
    for (int i = 0; i < 4; ++i) {
        pongFrame.push_back(0);
    }
    
    // Add payload
    pongFrame.insert(pongFrame.end(), pingData.begin(), pingData.end());
    
    return SendRawData(pongFrame.data(), pongFrame.size());
}

bool ArchipelagoSocket::SendHandshake() {
    // Create JSON handshake message for Archipelago
    std::stringstream json;
    json << "[{";
    json << "\"cmd\":\"Connect\",";
    json << "\"password\":\"" << m_password << "\",";
    json << "\"name\":\"" << m_slotName << "\",";
    json << "\"version\":{";
    json << "\"major\":0,\"minor\":6,\"build\":2,";  // Match server version
    json << "\"class\":\"Version\"";
    json << "},";
    json << "\"uuid\":\"selaco-" << GenerateUUID() << "\",";
    json << "\"game\":\"Selaco\",";
    json << "\"tags\":[],";  // Empty tags array - required by server
    json << "\"slot_data\":true,";
    json << "\"items_handling\":0";
    json << "}]";
    
    std::string jsonStr = json.str();
    
    if (archipelago_debug) {
        Printf("Sending Archipelago handshake\n");
    }
    
    // Send as WebSocket text frame
    if (!SendWebSocketFrame(jsonStr)) {
        m_lastError = "Failed to send handshake frame";
        return false;
    }
    
    Printf("Handshake sent successfully\n");
    return true;
}

bool ArchipelagoSocket::ProcessHandshakeResponse() {
    Printf("Waiting for Archipelago handshake response...\n");
    
    // Increase timeout for large messages
    #ifdef _WIN32
        DWORD timeout = 30000; // 30 seconds
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    #else
        struct timeval tv;
        tv.tv_sec = 30;
        tv.tv_usec = 0;
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    #endif
    
    // Also increase send buffer size for large messages
    int bufSize = 256 * 1024; // 256KB
    setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)&bufSize, sizeof(bufSize));
    setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char*)&bufSize, sizeof(bufSize));
    
    std::string response;
    
    // First response should be RoomInfo
    if (!ReceiveWebSocketFrame(response)) {
        m_lastError = "Failed to receive response frame";
        return false;
    }
    
    if (archipelago_debug) {
        // Don't print raw response to avoid binary output
        Printf("Received response (%zu bytes)\n", response.length());
    }
    
    // Check for RoomInfo (which we get first)
    if (response.find("\"cmd\":\"RoomInfo\"") != std::string::npos) {
        Printf("Received RoomInfo, waiting for Connected message...\n");
        
        // Clear response and wait for Connected message
        response.clear();
        if (!ReceiveWebSocketFrame(response)) {
            m_lastError = "Failed to receive Connected message";
            return false;
        }
        
        if (archipelago_debug) {
            Printf("Received Connected message (%zu bytes)\n", response.length());
        }
    }
    
    // Check response
    if (response.find("\"cmd\":\"Connected\"") != std::string::npos) {
        Printf(TEXTCOLOR_GREEN "Successfully connected to Archipelago as '%s'!\n", m_slotName.c_str());
        
        // Parse missing locations count if present
        size_t locPos = response.find("\"missing_locations\":[");
        if (locPos != std::string::npos) {
            size_t endPos = response.find("]", locPos);
            if (endPos != std::string::npos) {
                std::string locStr = response.substr(locPos + 21, endPos - locPos - 21);
                int locCount = std::count(locStr.begin(), locStr.end(), ',') + 1;
                Printf("Missing locations: %d\n", locCount);
            }
        }
        
        SetNonBlocking(true);
        return true;
    } else if (response.find("\"cmd\":\"ConnectionRefused\"") != std::string::npos) {
        m_lastError = "Connection refused: " + response;
        Printf(TEXTCOLOR_RED "%s\n", m_lastError.c_str());
        return false;
    }
    
    m_lastError = "Unexpected response";
    return false;
}

bool ArchipelagoSocket::SendMessage(const ArchipelagoMessage& msg) {
    if (!m_connected) {
        m_lastError = "Not connected";
        return false;
    }

    // Convert message to JSON format
    std::stringstream json;
    
    switch (msg.type) {
        case ArchipelagoMessageType::DATA:
            json << "[{\"cmd\":\"Say\",\"text\":\"" << msg.data << "\"}]";
            break;
            
        default:
            // Send as-is if already JSON
            json << msg.data;
            break;
    }
    
    std::string jsonStr = json.str();
    return SendWebSocketFrame(jsonStr);
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

bool ArchipelagoSocket::SendRawData(const void* data, size_t size) {
    const char* buffer = static_cast<const char*>(data);
    size_t totalSent = 0;
    
    while (totalSent < size) {
        int sent = send(m_socket, buffer + totalSent, size - totalSent, 0);
        if (sent <= 0) {
            m_lastError = "Socket send error";
            return false;
        }
        totalSent += sent;
    }
    
    return true;
}

bool ArchipelagoSocket::ReceiveRawData(void* data, size_t size) {
    uint8_t* buffer = static_cast<uint8_t*>(data);
    size_t totalReceived = 0;
    
    while (totalReceived < size) {
        int received = recv(m_socket, reinterpret_cast<char*>(buffer + totalReceived), 
                           size - totalReceived, 0);
        
        if (received <= 0) {
            if (received == 0) {
                m_lastError = "Connection closed by peer";
            } else {
                #ifdef _WIN32
                    int err = WSAGetLastError();
                    if (err == WSAEWOULDBLOCK && m_connected) {
                        // Non-blocking mode, retry
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        continue;
                    }
                #else
                    if (errno == EWOULDBLOCK && m_connected) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        continue;
                    }
                #endif
                m_lastError = "Socket receive error";
            }
            return false;
        }
        
        totalReceived += received;
    }
    
    return true;
}

void ArchipelagoSocket::SetNonBlocking(bool enable) {
#ifdef _WIN32
    u_long mode = enable ? 1 : 0;
    ioctlsocket(m_socket, FIONBIO, &mode);
#else
    int flags = fcntl(m_socket, F_GETFL, 0);
    if (enable) {
        fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
    } else {
        fcntl(m_socket, F_SETFL, flags & ~O_NONBLOCK);
    }
#endif
}

std::string ArchipelagoSocket::GetConnectionInfo() const {
    if (!m_connected) {
        return "Not connected";
    }
    
    std::stringstream ss;
    ss << "Connected to " << m_host << ":" << m_port;
    return ss.str();
}

void ArchipelagoSocket::ReceiverThreadFunc() {
    SetNonBlocking(false); // Use blocking mode for receiver thread
    Printf("Receiver thread started\n");
    
    // Increase thread-local timeout
    #ifdef _WIN32
        DWORD timeout = 100; // 100ms for checking m_shouldStop
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    #else
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    #endif
    
    while (!m_shouldStop && m_connected) {
        std::string message;
        
        if (ReceiveWebSocketFrame(message)) {
            if (!message.empty()) {
                // Parse and queue the message
                ArchipelagoMessage msg;
                if (ParseJsonMessage(message, msg)) {
                    std::lock_guard<std::mutex> lock(m_recvMutex);
                    m_recvQueue.push(msg);
                }
            }
        }
    }
    
    Printf("Receiver thread ended\n");
}

bool ArchipelagoSocket::ParseJsonMessage(const std::string& json, ArchipelagoMessage& msg) {
    // Simple JSON parsing for Archipelago messages
    if (json.find("\"cmd\":\"Print\"") != std::string::npos || 
        json.find("\"cmd\":\"PrintJSON\"") != std::string::npos) {
        msg.type = ArchipelagoMessageType::PRINT;
        msg.data = json;
        return true;
    }
    
    if (json.find("\"cmd\":\"Connected\"") != std::string::npos) {
        msg.type = ArchipelagoMessageType::CONNECTED;
        msg.data = json;
        return true;
    }
    
    if (json.find("\"cmd\":\"ConnectionRefused\"") != std::string::npos) {
        msg.type = ArchipelagoMessageType::REJECTED;
        msg.data = json;
        return true;
    }
    
    if (json.find("\"cmd\":\"ReceivedItems\"") != std::string::npos) {
        msg.type = ArchipelagoMessageType::DATA;
        msg.data = json;
        return true;
    }
    
    // Default to PRINT for unknown messages
    msg.type = ArchipelagoMessageType::PRINT;
    msg.data = json;
    return true;
}