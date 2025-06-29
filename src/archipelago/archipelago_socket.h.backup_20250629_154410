#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <thread>
#include <json/json.h>  // vcpkg style include
#include <ixwebsocket/IXWebSocket.h>

// Forward declare Printf if not available
#ifndef Printf
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
    Json::Value json;
};

class ArchipelagoSocket {
public:
    ArchipelagoSocket();
    ~ArchipelagoSocket();
    
    // Connect with slot name and optional password
    bool Connect(const std::string& host, uint16_t port, 
                 const std::string& slotName, 
                 const std::string& password = "");
    
    bool SendMessage(const ArchipelagoMessage& msg);
    bool SendJson(const Json::Value& json);
    bool ReceiveMessage(ArchipelagoMessage& msg);
    bool HasPendingMessages() const;
    
    bool IsConnected() const { return m_connected && m_authenticated; }
    bool IsSocketConnected() const { return m_connected; }
    std::string GetConnectionInfo() const;
    std::string GetLastError() const { return m_lastError; }
    std::string GetSlotName() const { return m_slotName; }
    
private:
    
    bool SendHandshake();
    bool ProcessMessage(const std::string& message);
    bool WaitForConnection(int timeoutMs = 5000);
    bool WaitForAuthentication(int timeoutMs = 30000);
    std::string GenerateUUID();
    
    ix::WebSocket m_webSocket;
    std::atomic<bool> m_connected;
    std::atomic<bool> m_authenticated;
    std::atomic<bool> m_sslFailed;
    
    std::string m_host;
    uint16_t m_port;
    std::string m_slotName;
    std::string m_password;
    std::string m_lastError;
    
    // Server version info
    int m_serverVersionMajor;
    int m_serverVersionMinor;
    int m_serverVersionBuild;
    
    std::queue<ArchipelagoMessage> m_recvQueue;
    mutable std::mutex m_recvMutex;
    
    Json::CharReaderBuilder m_jsonReaderBuilder;
    Json::StreamWriterBuilder m_jsonWriterBuilder;
    std::unique_ptr<Json::CharReader> m_jsonReader;
    std::unique_ptr<Json::StreamWriter> m_jsonWriter;
};