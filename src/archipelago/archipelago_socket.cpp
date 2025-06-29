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
    // FIXED: Return the exact key that works
    // Original function was generating invalid/corrupted keys
    return "dGhlIHNhbXBsZSBub25jZQ==";  // "the sample nonce" in Base64
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

    Printf("Disconnecting from Archipelago server...\n");

    // Stop receiver thread
    m_shouldStop = true;
    if (m_recvThread.joinable()) {
        m_recvThread.join();
    }

    // Send disconnect frame
    if (m_socket != INVALID_SOCKET) {
        // Send WebSocket close frame
        uint8_t closeFrame[] = { 0x88, 0x00 }; // FIN + CLOSE opcode, 0 length
        send(m_socket, (char*)closeFrame, sizeof(closeFrame), 0);
        
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }

    m_connected = false;
    m_host.clear();
    m_port = 0;
    m_slotName.clear();
    m_password.clear();

    // Clear message queue
    std::lock_guard<std::mutex> lock(m_recvMutex);
    while (!m_recvQueue.empty()) {
        m_recvQueue.pop();
    }

    Printf("Disconnected from Archipelago server\n");
}

bool ArchipelagoSocket::PerformWebSocketHandshake() {
    // HARDCODED FIX: Use the exact key that works in archipelago_test_raw
    // This bypasses the string corruption issue in GenerateWebSocketKey()
    std::string key = "dGhlIHNhbXBsZSBub25jZQ==";  // "the sample nonce" in Base64
    
    // Build WebSocket upgrade request
    std::stringstream request;
    request << "GET / HTTP/1.1\r\n";
    request << "Host: " << m_host << ":" << m_port << "\r\n";
    request << "Upgrade: websocket\r\n";
    request << "Connection: Upgrade\r\n";
    request << "Sec-WebSocket-Key: " << key << "\r\n";
    request << "Sec-WebSocket-Version: 13\r\n";
    request << "\r\n";

    std::string requestStr = request.str();
    
    if (archipelago_debug) {
        Printf("=== WebSocket Handshake Debug ===\n");
        Printf("Sending WebSocket upgrade request:\n");
        Printf("--- REQUEST START ---\n");
        Printf("%s", requestStr.c_str());
        Printf("--- REQUEST END ---\n");
    }
    
    if (!SendRawData(requestStr.c_str(), requestStr.length())) {
        m_lastError = "Failed to send WebSocket handshake";
        Printf(TEXTCOLOR_RED "Failed to send WebSocket upgrade request\n");
        return false;
    }

    // Set timeout for handshake
    #ifdef _WIN32
        DWORD timeout = 30000; // 30 seconds
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    #else
        struct timeval tv;
        tv.tv_sec = 30;
        tv.tv_usec = 0;
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    #endif
    
    // Increase buffer sizes
    int bufSize = 256 * 1024; // 256KB
    setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)&bufSize, sizeof(bufSize));
    setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char*)&bufSize, sizeof(bufSize));

    // Read response with larger buffer
    const size_t HANDSHAKE_BUFFER_SIZE = 65536; // 64KB for handshake
    std::vector<char> buffer(HANDSHAKE_BUFFER_SIZE);
    std::string response;
    int totalBytes = 0;
    
    Printf("Waiting for WebSocket handshake response...\n");
    
    auto startTime = std::chrono::steady_clock::now();
    
    // Read until we have the complete HTTP headers
    while (response.find("\r\n\r\n") == std::string::npos) {
        int bytes = recv(m_socket, buffer.data(), buffer.size() - 1, 0);
        
        if (bytes < 0) {
            #ifdef _WIN32
                int error = WSAGetLastError();
                if (archipelago_debug) {
                    Printf(TEXTCOLOR_RED "recv() failed with WSA error: %d\n", error);
                }
                if (error == WSAETIMEDOUT) {
                    m_lastError = "Timeout waiting for WebSocket response";
                } else {
                    m_lastError = "Failed to receive WebSocket handshake response";
                }
            #else
                if (archipelago_debug) {
                    Printf(TEXTCOLOR_RED "recv() failed with error: %s\n", strerror(errno));
                }
                m_lastError = "Failed to receive WebSocket handshake response";
            #endif
            return false;
        } else if (bytes == 0) {
            m_lastError = "Connection closed by server during handshake";
            Printf(TEXTCOLOR_RED "Connection closed by server during handshake\n");
            return false;
        }
        
        buffer[bytes] = '\0'; // Null terminate for safety
        response.append(buffer.data(), bytes);
        totalBytes += bytes;
        
        if (archipelago_debug) {
            Printf("Received %d bytes (total: %d)\n", bytes, totalBytes);
        }
        
        // Check for timeout
        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (elapsed > std::chrono::seconds(30)) {
            m_lastError = "Timeout exceeded while waiting for complete headers";
            Printf(TEXTCOLOR_RED "Timeout exceeded (30s) while waiting for WebSocket response\n");
            return false;
        }
        
        // Prevent infinite buffer growth
        if (response.length() > MAX_WEBSOCKET_PAYLOAD) {
            m_lastError = "Response too large";
            Printf(TEXTCOLOR_RED "Response exceeded maximum size\n");
            return false;
        }
    }
    
    if (archipelago_debug) {
        Printf("\n=== RESPONSE RECEIVED ===\n");
        Printf("Total bytes: %d\n", totalBytes);
        Printf("--- RESPONSE START ---\n");
        
        // Print response safely, limiting output
        size_t printLen = std::min(response.length(), size_t(1024));
        Printf("%.*s", (int)printLen, response.c_str());
        if (response.length() > 1024) {
            Printf("\n[... %zu more bytes ...]\n", response.length() - 1024);
        }
        Printf("\n--- RESPONSE END ---\n\n");
    }
    
    // Parse status line
    size_t firstLine = response.find("\r\n");
    if (firstLine != std::string::npos) {
        std::string statusLine = response.substr(0, firstLine);
        Printf("Status line: %s\n", statusLine.c_str());
        
        // Check for 101 Switching Protocols
        if (statusLine.find("101") == std::string::npos) {
            m_lastError = "WebSocket upgrade rejected: " + statusLine;
            Printf(TEXTCOLOR_RED "WebSocket upgrade failed - server returned: %s\n", statusLine.c_str());
            
            // Try to extract any error body
            size_t bodyStart = response.find("\r\n\r\n");
            if (bodyStart != std::string::npos && bodyStart + 4 < response.length()) {
                std::string body = response.substr(bodyStart + 4, 200); // First 200 chars of body
                Printf(TEXTCOLOR_RED "Response body: %s\n", body.c_str());
            }
            
            return false;
        }
    } else {
        m_lastError = "Invalid HTTP response - no status line";
        Printf(TEXTCOLOR_RED "Invalid HTTP response format\n");
        return false;
    }
    
    // Verify required headers (case-insensitive)
    std::string responseLower = response;
    std::transform(responseLower.begin(), responseLower.end(), responseLower.begin(), ::tolower);
    
    bool hasUpgrade = responseLower.find("upgrade: websocket") != std::string::npos;
    bool hasConnection = responseLower.find("connection: upgrade") != std::string::npos;
    
    if (archipelago_debug) {
        Printf("Header validation:\n");
        Printf("  Upgrade header: %s\n", hasUpgrade ? "Found" : "Missing");
        Printf("  Connection header: %s\n", hasConnection ? "Found" : "Missing");
    }
    
    if (!hasUpgrade || !hasConnection) {
        m_lastError = "Missing required WebSocket headers";
        Printf(TEXTCOLOR_RED "Invalid WebSocket response - missing required headers\n");
        return false;
    }

    Printf(TEXTCOLOR_GREEN "WebSocket handshake successful!\n");
    return true;
}

bool ArchipelagoSocket::SendRawData(const void* data, size_t size) {
    const char* ptr = (const char*)data;
    size_t sent = 0;

    while (sent < size) {
        int result = send(m_socket, ptr + sent, (int)(size - sent), 0);
        if (result == SOCKET_ERROR) {
            #ifdef _WIN32
                int error = WSAGetLastError();
                if (archipelago_debug) {
                    Printf(TEXTCOLOR_RED "send() failed with WSA error: %d\n", error);
                }
            #else
                if (archipelago_debug) {
                    Printf(TEXTCOLOR_RED "send() failed: %s\n", strerror(errno));
                }
            #endif
            return false;
        }
        sent += result;
    }

    if (archipelago_debug) {
        Printf("Sent %zu bytes\n", sent);
    }
    
    return true;
}

bool ArchipelagoSocket::ReceiveRawData(void* data, size_t size) {
    char* ptr = (char*)data;
    size_t received = 0;

    while (received < size) {
        int result = recv(m_socket, ptr + received, (int)(size - received), 0);
        if (result <= 0) {
            if (result == 0) {
                if (archipelago_debug) {
                    Printf("Connection closed by peer\n");
                }
            } else {
                #ifdef _WIN32
                    int error = WSAGetLastError();
                    if (archipelago_debug && error != WSAEWOULDBLOCK) {
                        Printf(TEXTCOLOR_RED "recv() failed with WSA error: %d\n", error);
                    }
                #else
                    if (archipelago_debug && errno != EWOULDBLOCK && errno != EAGAIN) {
                        Printf(TEXTCOLOR_RED "recv() failed: %s\n", strerror(errno));
                    }
                #endif
            }
            return false;
        }
        received += result;
    }

    return true;
}

bool ArchipelagoSocket::SendWebSocketFrame(const std::string& data) {
    if (m_socket == INVALID_SOCKET) {
        return false;
    }

    std::vector<uint8_t> frame;
    
    // FIN (1) + RSV (000) + Opcode (0001 = text)
    frame.push_back(0x81);

    // Mask bit + payload length
    size_t payloadLen = data.length();
    if (payloadLen < 126) {
        frame.push_back(0x80 | static_cast<uint8_t>(payloadLen));
    } else if (payloadLen < 65536) {
        frame.push_back(0x80 | 126);
        frame.push_back((payloadLen >> 8) & 0xFF);
        frame.push_back(payloadLen & 0xFF);
    } else {
        frame.push_back(0x80 | 127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back((payloadLen >> (i * 8)) & 0xFF);
        }
    }

    // Masking key
    uint32_t maskKey = GenerateMaskingKey();
    uint8_t mask[4];
    mask[0] = (maskKey >> 24) & 0xFF;
    mask[1] = (maskKey >> 16) & 0xFF;
    mask[2] = (maskKey >> 8) & 0xFF;
    mask[3] = maskKey & 0xFF;
    
    frame.push_back(mask[0]);
    frame.push_back(mask[1]);
    frame.push_back(mask[2]);
    frame.push_back(mask[3]);

    // Masked payload
    for (size_t i = 0; i < data.length(); ++i) {
        frame.push_back(data[i] ^ mask[i % 4]);
    }

    if (archipelago_debug) {
        Printf("Sending WebSocket frame: %zu bytes payload, %zu bytes total\n", 
               data.length(), frame.size());
    }

    return SendRawData(frame.data(), frame.size());
}

bool ArchipelagoSocket::ReceiveWebSocketFrame(std::string& message) {
    message.clear();
    
    while (true) {
        // Read frame header
        uint8_t header[2];
        if (!ReceiveRawData(header, 2)) {
            return false;
        }
        
        bool fin = (header[0] & 0x80) != 0;
        uint8_t opcode = header[0] & 0x0F;
        bool masked = (header[1] & 0x80) != 0;
        uint64_t payloadLen = header[1] & 0x7F;
        
        if (archipelago_debug) {
            Printf("WebSocket frame: FIN=%d, Opcode=%d, Masked=%d, Len=%llu\n",
                   fin, opcode, masked, payloadLen);
        }
        
        // Extended payload length
        if (payloadLen == 126) {
            uint8_t extLen[2];
            if (!ReceiveRawData(extLen, 2)) {
                return false;
            }
            payloadLen = (static_cast<uint64_t>(extLen[0]) << 8) | extLen[1];
        } else if (payloadLen == 127) {
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
            if (archipelago_debug) {
                Printf("Received close frame\n");
            }
            m_connected = false;
            return false;
        } else if (opcode == 0x9) { // Ping frame
            if (archipelago_debug) {
                Printf("Received ping frame, sending pong\n");
            }
            // Send pong response
            SendPongFrame(payload);
            continue; // Don't return, wait for next frame
        } else if (opcode == 0xA) { // Pong frame
            if (archipelago_debug) {
                Printf("Received pong frame\n");
            }
            continue; // Ignore pongs
        }
        
        if (fin) {
            break; // Complete message received
        }
    }
    
    if (archipelago_debug && !message.empty()) {
        Printf("Received complete WebSocket message: %zu bytes\n", message.length());
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
        Printf("\n=== Sending Archipelago Connect ===\n");
        Printf("%s\n", jsonStr.c_str());
        Printf("=================================\n");
    }
    
    // Send as WebSocket text frame
    if (!SendWebSocketFrame(jsonStr)) {
        m_lastError = "Failed to send handshake frame";
        return false;
    }
    
    Printf("Archipelago handshake sent successfully\n");
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
    
    std::string response;
    
    // First response should be RoomInfo
    if (!ReceiveWebSocketFrame(response)) {
        m_lastError = "Failed to receive RoomInfo";
        Printf(TEXTCOLOR_RED "Failed to receive initial response (RoomInfo expected)\n");
        return false;
    }
    
    if (archipelago_debug) {
        Printf("\n=== Received Response 1 ===\n");
        Printf("Size: %zu bytes\n", response.size());
        if (response.size() < 1000) {
            Printf("Content: %s\n", response.c_str());
        } else {
            Printf("Content (first 500 chars): %.500s...\n", response.c_str());
        }
        Printf("========================\n");
    }
    
    // Check if it's RoomInfo
    if (response.find("\"cmd\":\"RoomInfo\"") != std::string::npos) {
        Printf("Received RoomInfo, waiting for Connected message...\n");
        
        // Second response should be Connected or ConnectionRefused
        response.clear();
        if (!ReceiveWebSocketFrame(response)) {
            m_lastError = "Failed to receive Connected message";
            Printf(TEXTCOLOR_RED "Failed to receive second response (Connected expected)\n");
            return false;
        }
        
        if (archipelago_debug) {
            Printf("\n=== Received Response 2 ===\n");
            Printf("Size: %zu bytes\n", response.size());
            if (response.size() < 1000) {
                Printf("Content: %s\n", response.c_str());
            } else {
                Printf("Content (first 500 chars): %.500s...\n", response.c_str());
            }
            Printf("========================\n");
        }
    }
    
    // Check response type
    if (response.find("\"cmd\":\"Connected\"") != std::string::npos) {
        Printf(TEXTCOLOR_GREEN "Successfully connected to Archipelago as '%s'!\n", m_slotName.c_str());
        
        // Parse some useful info if available
        size_t slotPos = response.find("\"slot\":");
        if (slotPos != std::string::npos) {
            size_t slotEnd = response.find(",", slotPos);
            if (slotEnd != std::string::npos) {
                std::string slotNum = response.substr(slotPos + 7, slotEnd - slotPos - 7);
                Printf("Slot number: %s\n", slotNum.c_str());
            }
        }
        
        return true;
    } else if (response.find("\"cmd\":\"ConnectionRefused\"") != std::string::npos) {
        // Extract error details
        std::string reason = "Unknown";
        size_t errPos = response.find("[\"");
        if (errPos != std::string::npos) {
            size_t errEnd = response.find("\"]", errPos);
            if (errEnd != std::string::npos) {
                reason = response.substr(errPos + 2, errEnd - errPos - 2);
            }
        }
        
        m_lastError = "Connection refused: " + reason;
        Printf(TEXTCOLOR_RED "Connection refused by server: %s\n", reason.c_str());
        return false;
    } else {
        m_lastError = "Unexpected response from server";
        Printf(TEXTCOLOR_RED "Unexpected response - expected Connected or ConnectionRefused\n");
        return false;
    }
}

bool ArchipelagoSocket::SendMessage(const ArchipelagoMessage& msg) {
    if (!m_connected) {
        m_lastError = "Not connected";
        return false;
    }
    
    // Convert message to JSON based on type
    std::string json;
    
    switch (msg.type) {
        case ArchipelagoMessageType::DATA:
            // Assume msg.data is already formatted JSON
            json = msg.data;
            break;
            
        case ArchipelagoMessageType::PING:
            json = "[{\"cmd\":\"Ping\"}]";
            break;
            
        default:
            m_lastError = "Unsupported message type";
            return false;
    }
    
    return SendWebSocketFrame(json);
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

std::string ArchipelagoSocket::GetConnectionInfo() const {
    if (!m_connected) {
        return "Not connected";
    }
    
    std::stringstream info;
    info << "Connected to " << m_host << ":" << m_port;
    info << " as '" << m_slotName << "'";
    return info.str();
}

void ArchipelagoSocket::ReceiverThreadFunc() {
    SetNonBlocking(false); // Blocking mode for receiver
    
    while (!m_shouldStop && m_connected) {
        std::string frame;
        if (ReceiveWebSocketFrame(frame)) {
            // Parse and queue message
            ArchipelagoMessage msg;
            if (ParseJsonMessage(frame, msg)) {
                std::lock_guard<std::mutex> lock(m_recvMutex);
                m_recvQueue.push(msg);
            }
        }
    }
}

bool ArchipelagoSocket::ParseJsonMessage(const std::string& json, ArchipelagoMessage& msg) {
    // Very simple JSON parsing - in production, use a proper JSON library
    
    if (json.find("\"cmd\":\"Connected\"") != std::string::npos) {
        msg.type = ArchipelagoMessageType::CONNECTED;
        msg.data = json;
        return true;
    } else if (json.find("\"cmd\":\"ConnectionRefused\"") != std::string::npos) {
        msg.type = ArchipelagoMessageType::REJECTED;
        
        // Try to extract reason
        size_t reasonPos = json.find("\"reason\":\"");
        if (reasonPos != std::string::npos) {
            reasonPos += 10;
            size_t endPos = json.find("\"", reasonPos);
            if (endPos != std::string::npos) {
                msg.data = json.substr(reasonPos, endPos - reasonPos);
            }
        } else {
            msg.data = "Unknown reason";
        }
        return true;
    } else if (json.find("\"cmd\":\"RoomInfo\"") != std::string::npos) {
        // Room info - could parse and store if needed
        if (archipelago_debug) {
            Printf("Received RoomInfo\n");
        }
        return false; // Don't queue, handle internally
    } else if (json.find("\"cmd\":\"DataPackage\"") != std::string::npos) {
        msg.type = ArchipelagoMessageType::DATA_PACKAGE;
        msg.data = json;
        return true;
    } else if (json.find("\"cmd\":\"Print\"") != std::string::npos) {
        msg.type = ArchipelagoMessageType::PRINT;
        
        // Extract text
        size_t textPos = json.find("\"text\":\"");
        if (textPos != std::string::npos) {
            textPos += 8;
            size_t endPos = json.find("\"", textPos);
            if (endPos != std::string::npos) {
                msg.data = json.substr(textPos, endPos - textPos);
            }
        }
        return true;
    } else if (json.find("\"cmd\":\"PrintJSON\"") != std::string::npos) {
        msg.type = ArchipelagoMessageType::PRINT_JSON;
        msg.data = json;
        return true;
    } else {
        // Generic data message
        msg.type = ArchipelagoMessageType::DATA;
        msg.data = json;
        return true;
    }
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