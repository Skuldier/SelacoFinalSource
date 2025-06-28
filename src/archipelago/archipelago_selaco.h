#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <random>

// Forward declarations
class ArchipelagoSocket;
struct ArchipelagoMessage;

namespace Archipelago {

// Forward declarations from other headers
struct ItemDef;
struct LocationDef;

class SelacoArchipelago {
public:
    SelacoArchipelago();
    ~SelacoArchipelago();
    
    // Core functionality
    bool Initialize();
    void Shutdown();
    bool Connect(const std::string& host, uint16_t port, 
                const std::string& slot_name, const std::string& password = "");
    void Disconnect();
    void ProcessMessages();
    
    // Game integration
    void CheckLocation(int location_id);
    void ReceiveItem(int item_id, int sender_slot);
    void SendDeathLink(const std::string& cause);
    
    // Status
    bool IsConnected() const { return m_connected; }
    const std::string& GetSlotName() const { return m_slot_name; }
    int GetClearanceLevel() const { return m_clearance_level; }
    int GetCabinetCards() const { return m_cabinet_cards; }
    
    // Console command handlers
    static void CMD_CheckLocation(int location_id);
    static void CMD_ListLocations(const std::string& map_name = "");
    static void CMD_SendItem(int item_id);
    
private:
    // Network
    std::unique_ptr<ArchipelagoSocket> m_socket;
    bool m_connected;
    std::string m_slot_name;
    
    // Game state
    std::unordered_map<int, ItemDef> m_items;
    std::unordered_map<int, LocationDef> m_locations;
    std::vector<int> m_checked_locations;
    std::vector<int> m_received_items;
    int m_clearance_level;
    int m_cabinet_cards;
    
    // Message handling
    void HandleMessage(const ArchipelagoMessage& msg);
    void HandleReceivedItems(const std::string& data);
    void HandleLocationInfo(const std::string& data);
    void HandleRoomUpdate(const std::string& data);
    void HandlePrintJSON(const std::string& data);
    void HandleDataPackage(const std::string& data);
    void HandleDeathLink(const std::string& data);
    
    // Networking
    void SendLocationCheck(int location_id);
    void SendConnectPacket();
    
    // Item handling
    void GiveItemToPlayer(const ItemDef& item);
    int GetAmountForItem(const ItemDef& item);
    
    // Data loading
    void LoadItemData();
    void LoadLocationData();
    
    // Utilities
    std::string GenerateUUID();
};

// Global functions for integration
void Archipelago_Selaco_Init();
void Archipelago_Selaco_Shutdown();
void Archipelago_Selaco_ProcessMessages();
bool Archipelago_Selaco_IsConnected();

} // namespace Archipelago

// Integration points for d_main.cpp
extern "C" {
    void Archipelago_Selaco_OnLevelStart(const char* map_name);
    void Archipelago_Selaco_OnLevelEnd();
    void Archipelago_Selaco_OnItemPickup(const char* item_class, int amount);
    void Archipelago_Selaco_OnPlayerDeath(const char* cause);
    void Archipelago_Selaco_OnSecretFound(int secret_id);
    void Archipelago_Selaco_OnKeyCardUsed(int keycard_type);
    void Archipelago_Selaco_OnCabinetOpened(int cabinet_id);
    void Archipelago_Selaco_OnBossDefeated(const char* boss_name);
}