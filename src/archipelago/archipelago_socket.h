#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
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
#endif

// Archipelago message types
enum class ArchipelagoMessageType : uint8_t {
    CONNECT = 0x01,
    DISCONNECT = 0x02,
    PING = 0x03,
    PONG = 0x04,
    DATA = 0x05,
    MSG_ERROR = 0xFF  // ERROR conflicts with Windows macro
};

// Basic message structure
struct ArchipelagoMessage {
    ArchipelagoMessageType type;
    std::string data;
};

class ArchipelagoSocket {
public:
    ArchipelagoSocket();
    ~ArchipelagoSocket();

    // Connection management
    bool Connect(const std::string& host, uint16_t port);
    void Disconnect();
    bool IsConnected() const { return m_connected; }

    // Message handling
    bool SendMessage(const ArchipelagoMessage& msg);
    bool ReceiveMessage(ArchipelagoMessage& msg);
    bool HasPendingMessages() const;

    // Status
    std::string GetLastError() const { return m_lastError; }
    std::string GetConnectionInfo() const;

private:
    SOCKET m_socket;
    std::atomic<bool> m_connected;
    std::string m_host;
    uint16_t m_port;
    std::string m_lastError;
    
    // Thread-safe message queue
    mutable std::mutex m_recvMutex;
    std::queue<ArchipelagoMessage> m_recvQueue;
    
    // Background receive thread
    std::thread m_recvThread;
    std::atomic<bool> m_shouldStop;
    
    void ReceiverThreadFunc();
    bool SendRawData(const void* data, size_t size);
    bool ReceiveRawData(void* data, size_t size);
    void SetNonBlocking(bool enable);
    
    // Platform-specific initialization
    static bool InitializeSockets();
    static void CleanupSockets();
    static int s_socketsInitialized;
};