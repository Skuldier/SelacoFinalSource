#pragma once

#include <string>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <set>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET socket_t;
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <errno.h>
    typedef int socket_t;
    #define INVALID_SOCKET_VALUE -1
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
    #define WSAEWOULDBLOCK EWOULDBLOCK
#endif

enum class ArchipelagoMessageType : uint8_t {
    CONNECT = 0,
    CONNECTED = 1,
    REJECTED = 2,
    DATA_PACKAGE = 3,
    PRINT = 4,
    PRINT_JSON = 5,
    DATA = 6,
    BOUNCE = 7,
    GET = 8,
    SET = 9,
    SET_REPLY = 10,
    DISCONNECT = 11,
    PING = 12,
    PONG = 13,
    MSG_ERROR = 0xFF
};

struct ArchipelagoMessage {
    ArchipelagoMessageType type;
    std::string data;
};

class ArchipelagoSocket {
public:
    ArchipelagoSocket();
    ~ArchipelagoSocket();
    
    // Connect with slot name and optional password
    bool Connect(const std::string& host, uint16_t port, 
                 const std::string& slotName, 
                 const std::string& password = "");
    void Disconnect();
    
    bool SendMessage(const ArchipelagoMessage& msg);
    bool ReceiveMessage(ArchipelagoMessage& msg);
    bool HasPendingMessages() const;
    
    bool IsConnected() const { return m_connected; }
    std::string GetConnectionInfo() const;
    std::string GetLastError() const { return m_lastError; }
    std::string GetSlotName() const { return m_slotName; }
    
private:
    // WebSocket operations
    bool PerformWebSocketHandshake();
    bool SendWebSocketFrame(const std::string& data);
    bool ReceiveWebSocketFrame(std::string& data);
    bool SendPongFrame(const std::vector<uint8_t>& pingData);
    std::string GenerateWebSocketKey();
    uint32_t GenerateMaskingKey();
    
    // Archipelago protocol
    bool SendHandshake();
    bool ProcessHandshakeResponse();
    bool ParseJsonMessage(const std::string& json, ArchipelagoMessage& msg);
    void ReceiverThreadFunc();
    
    // Socket helpers
    bool SendRawData(const void* data, size_t size);
    bool ReceiveRawData(void* data, size_t size);
    void SetNonBlocking(bool enable);
    std::string GenerateUUID();
    
    static bool InitializeSockets();
    static void CleanupSockets();
    
    socket_t m_socket;
    std::atomic<bool> m_connected;
    std::string m_host;
    uint16_t m_port;
    std::string m_slotName;
    std::string m_password;
    std::string m_lastError;
    
    std::thread m_recvThread;
    std::atomic<bool> m_shouldStop;
    
    std::queue<ArchipelagoMessage> m_recvQueue;
    mutable std::mutex m_recvMutex;
    
    static int s_socketsInitialized;
};