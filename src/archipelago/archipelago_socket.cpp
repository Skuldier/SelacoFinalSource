#include "archipelago_socket.h"
#include "doomtype.h"
#include "c_dispatch.h"
#include <cstring>
#include <sstream>
#include <chrono>

#ifndef _WIN32
#include <fcntl.h>
#endif

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

bool ArchipelagoSocket::Connect(const std::string& host, uint16_t port) {
    if (m_connected) {
        m_lastError = "Already connected";
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

    // Connect
    Printf("Connecting to Archipelago server at %s:%d...\n", host.c_str(), port);
    
    if (connect(m_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        m_lastError = "Failed to connect to server";
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    m_host = host;
    m_port = port;
    m_connected = true;
    m_shouldStop = false;

    // Start receiver thread
    m_recvThread = std::thread(&ArchipelagoSocket::ReceiverThreadFunc, this);

    Printf(TEXTCOLOR_GREEN "Connected to Archipelago server!\n");
    
    // Send initial connect message
    ArchipelagoMessage connectMsg;
    connectMsg.type = ArchipelagoMessageType::CONNECT;
    connectMsg.data = "Selaco Client v1.0";
    SendMessage(connectMsg);

    return true;
}

void ArchipelagoSocket::Disconnect() {
    if (!m_connected) {
        return;
    }

    // Send disconnect message
    ArchipelagoMessage disconnectMsg;
    disconnectMsg.type = ArchipelagoMessageType::DISCONNECT;
    SendMessage(disconnectMsg);

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

bool ArchipelagoSocket::SendMessage(const ArchipelagoMessage& msg) {
    if (!m_connected) {
        m_lastError = "Not connected";
        return false;
    }

    // Simple protocol: [type:1][size:4][data:size]
    uint8_t type = static_cast<uint8_t>(msg.type);
    uint32_t size = static_cast<uint32_t>(msg.data.size());
    
    // Convert to network byte order
    uint32_t netSize = htonl(size);

    // Send header
    if (!SendRawData(&type, sizeof(type))) {
        return false;
    }
    if (!SendRawData(&netSize, sizeof(netSize))) {
        return false;
    }

    // Send data
    if (size > 0 && !SendRawData(msg.data.c_str(), size)) {
        return false;
    }

    return true;
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

void ArchipelagoSocket::ReceiverThreadFunc() {
    SetNonBlocking(true);
    
    while (!m_shouldStop && m_connected) {
        ArchipelagoMessage msg;
        
        // Try to receive message header
        uint8_t type;
        uint32_t netSize;
        
        if (ReceiveRawData(&type, sizeof(type))) {
            if (ReceiveRawData(&netSize, sizeof(netSize))) {
                uint32_t size = ntohl(netSize);
                
                if (size > 0 && size < 1024 * 1024) { // 1MB max message size
                    msg.type = static_cast<ArchipelagoMessageType>(type);
                    msg.data.resize(size);
                    
                    if (ReceiveRawData(&msg.data[0], size)) {
                        // Add to queue
                        std::lock_guard<std::mutex> lock(m_recvMutex);
                        m_recvQueue.push(msg);
                        
                        // Handle ping/pong internally
                        if (msg.type == ArchipelagoMessageType::PING) {
                            ArchipelagoMessage pong;
                            pong.type = ArchipelagoMessageType::PONG;
                            pong.data = msg.data;
                            SendMessage(pong);
                        }
                    }
                }
            }
        }
        
        // Small delay to prevent CPU spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool ArchipelagoSocket::SendRawData(const void* data, size_t size) {
    const char* ptr = static_cast<const char*>(data);
    size_t sent = 0;
    
    while (sent < size) {
        int result = send(m_socket, ptr + sent, static_cast<int>(size - sent), 0);
        if (result == SOCKET_ERROR) {
            m_lastError = "Send failed";
            return false;
        }
        sent += result;
    }
    
    return true;
}

bool ArchipelagoSocket::ReceiveRawData(void* data, size_t size) {
    char* ptr = static_cast<char*>(data);
    size_t received = 0;
    
    while (received < size) {
        int result = recv(m_socket, ptr + received, static_cast<int>(size - received), 0);
        if (result == SOCKET_ERROR) {
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
#else
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
#endif
                return false; // No data available
            }
            m_lastError = "Receive failed";
            return false;
        } else if (result == 0) {
            // Connection closed
            m_connected = false;
            return false;
        }
        received += result;
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