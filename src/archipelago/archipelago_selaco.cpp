#include "archipelago_selaco.h"
#include "archipelago_socket.h"
#include "archipelago_items.h"
#include "archipelago_locations.h"
#include "c_dispatch.h"
#include "c_cvars.h"
#include "doomtype.h"
#include <memory>
#include <algorithm>
#include <sstream>
#include <random>

namespace Archipelago {

// Singleton instance
static std::unique_ptr<SelacoArchipelago> g_archipelago;

// CVars
CVAR(Bool, archipelago_enabled, false, CVAR_ARCHIVE)
CVAR(Bool, archipelago_deathlink, false, CVAR_ARCHIVE)
CVAR(Bool, archipelago_goal_completion, false, CVAR_ARCHIVE)

SelacoArchipelago::SelacoArchipelago() 
    : m_socket(std::make_unique<ArchipelagoSocket>()),
      m_connected(false),
      m_clearance_level(0),
      m_cabinet_cards(0) {
}

SelacoArchipelago::~SelacoArchipelago() {
    Disconnect();
}

bool SelacoArchipelago::Initialize() {
    if (!archipelago_enabled) {
        return false;
    }
    
    Printf("Initializing Archipelago integration for Selaco...\n");
    
    // Load item and location data
    LoadItemData();
    LoadLocationData();
    
    return true;
}

void SelacoArchipelago::Shutdown() {
    Disconnect();
    m_items.clear();
    m_locations.clear();
    m_checked_locations.clear();
    m_received_items.clear();
}

bool SelacoArchipelago::Connect(const std::string& host, uint16_t port, 
                               const std::string& slot_name, const std::string& password) {
    if (m_connected) {
        Printf("Already connected to Archipelago\n");
        return false;
    }
    
    if (!m_socket->Connect(host, port, slot_name, password)) {
        Printf(TEXTCOLOR_RED "Failed to connect to Archipelago: %s\n", 
               m_socket->GetLastError().c_str());
        return false;
    }
    
    m_connected = true;
    m_slot_name = slot_name;
    
    // Send initial connection data
    SendConnectPacket();
    
    return true;
}

void SelacoArchipelago::Disconnect() {
    if (!m_connected) {
        return;
    }
    
    m_socket->Disconnect();
    m_connected = false;
}

void SelacoArchipelago::ProcessMessages() {
    if (!m_connected) {
        return;
    }
    
    ArchipelagoMessage msg;
    while (m_socket->ReceiveMessage(msg)) {
        HandleMessage(msg);
    }
}

void SelacoArchipelago::HandleMessage(const ArchipelagoMessage& msg) {
    // Parse JSON message and handle different command types
    // This is simplified - real implementation would use a JSON parser
    
    std::string data = msg.data;
    
    if (data.find("\"cmd\":\"ReceivedItems\"") != std::string::npos) {
        HandleReceivedItems(data);
    }
    else if (data.find("\"cmd\":\"LocationInfo\"") != std::string::npos) {
        HandleLocationInfo(data);
    }
    else if (data.find("\"cmd\":\"RoomUpdate\"") != std::string::npos) {
        HandleRoomUpdate(data);
    }
    else if (data.find("\"cmd\":\"PrintJSON\"") != std::string::npos) {
        HandlePrintJSON(data);
    }
    else if (data.find("\"cmd\":\"DataPackage\"") != std::string::npos) {
        HandleDataPackage(data);
    }
    else if (archipelago_deathlink && data.find("\"cmd\":\"Bounce\"") != std::string::npos 
             && data.find("\"DeathLink\"") != std::string::npos) {
        HandleDeathLink(data);
    }
}

void SelacoArchipelago::CheckLocation(int location_id) {
    // Check if already checked
    if (std::find(m_checked_locations.begin(), m_checked_locations.end(), location_id) 
        != m_checked_locations.end()) {
        return;
    }
    
    // Add to checked locations
    m_checked_locations.push_back(location_id);
    
    // Find location info
    auto it = m_locations.find(location_id);
    if (it != m_locations.end()) {
        Printf(TEXTCOLOR_GREEN "Location checked: %s\n", it->second.name.c_str());
    }
    
    // Send location check to server
    SendLocationCheck(location_id);
}

void SelacoArchipelago::ReceiveItem(int item_id, int sender_slot) {
    // Find item info
    auto it = m_items.find(item_id);
    if (it == m_items.end()) {
        Printf(TEXTCOLOR_RED "Unknown item received: %d\n", item_id);
        return;
    }
    
    const ItemDef& item = it->second;
    m_received_items.push_back(item_id);
    
    Printf(TEXTCOLOR_GOLD "Received %s from slot %d\n", item.name.c_str(), sender_slot);
    
    // Give the item to the player
    GiveItemToPlayer(item);
}

void SelacoArchipelago::GiveItemToPlayer(const ItemDef& item) {
    // This would interface with Selaco's inventory system
    std::stringstream cmd;
    
    // Special handling for different item types
    switch (item.category) {
        case ItemCategory::PROGRESSION:
            if (item.internal_name == "SecurityCard") {
                m_clearance_level++;
                cmd << "give ClearanceLevel 1";
            } else if (item.internal_name == "CabinetCard") {
                m_cabinet_cards++;
                cmd << "give CabinetCardCount 1";
            } else {
                cmd << "give " << item.internal_name << " 1";
            }
            break;
            
        case ItemCategory::WEAPON:
            cmd << "give " << item.internal_name << " 1";
            break;
            
        case ItemCategory::WEAPON_UPGRADE:
            // Weapon upgrades need special handling
            cmd << "give " << item.internal_name << " 1";
            cmd << "; archipelago_enable_upgrade " << item.internal_name;
            break;
            
        case ItemCategory::HEALTH:
        case ItemCategory::ARMOR:
        case ItemCategory::AMMO:
        case ItemCategory::CONSUMABLE:
            if (item.max_quantity > 1) {
                // For stackable items, give a reasonable amount
                int amount = GetAmountForItem(item);
                cmd << "give " << item.internal_name << " " << amount;
            } else {
                cmd << "give " << item.internal_name << " 1";
            }
            break;
            
        default:
            cmd << "give " << item.internal_name << " 1";
            break;
    }
    
    // Execute the give command
    C_DoCommand(cmd.str().c_str());
}

int SelacoArchipelago::GetAmountForItem(const ItemDef& item) {
    // Determine appropriate amounts for different item types
    switch (item.id) {
        // Health
        case 5001: return 10;  // Small health
        case 5002: return 25;  // Large health
        case 5003: return 100; // Ultra health
        
        // Armor  
        case 6001: return 25;  // Light armor
        case 6002: return 50;  // Combat armor
        case 6003: return 75;  // Heavy armor
        case 6004: return 100; // Admiral armor
        case 6005: return 5;   // Armor shard
        
        // Ammo - give roughly half a magazine
        case 7001: return 9;   // Pistol ammo
        case 7002: return 4;   // Shotgun small
        case 7003: return 8;   // Shotgun medium
        case 7004: return 20;  // Shotgun large
        case 7005: return 15;  // Rifle small
        case 7006: return 30;  // Rifle medium
        case 7007: return 60;  // Rifle large
        case 7008: return 2;   // Grenade ammo
        case 7009: return 20;  // Nailgun small
        case 7010: return 50;  // Nailgun medium
        case 7011: return 10;  // DMR ammo
        case 7012: return 35;  // Plasma small
        case 7013: return 70;  // Plasma large
        case 7014: return 2;   // Railgun slug
        
        // Consumables
        case 9001: return 5;   // Credits small
        case 9002: return 25;  // Credits medium
        case 9003: return 100; // Credits large
        case 9004: return 15;  // Weapon parts
        
        default: return 1;
    }
}

void SelacoArchipelago::SendLocationCheck(int location_id) {
    if (!m_connected) return;
    
    std::stringstream json;
    json << "[{";
    json << "\"cmd\":\"LocationChecks\",";
    json << "\"locations\":[" << location_id << "]";
    json << "}]";
    
    ArchipelagoMessage msg;
    msg.type = ArchipelagoMessageType::DATA;
    msg.data = json.str();
    
    m_socket->SendMessage(msg);
}

void SelacoArchipelago::SendConnectPacket() {
    if (!m_connected) return;
    
    std::stringstream json;
    json << "[{";
    json << "\"cmd\":\"Connect\",";
    json << "\"game\":\"Selaco\",";
    json << "\"name\":\"" << m_slot_name << "\",";
    json << "\"uuid\":\"" << GenerateUUID() << "\",";
    json << "\"version\":{\"major\":0,\"minor\":1,\"build\":0},";
    json << "\"items_handling\":0b111,";  // All items handling
    json << "\"tags\":[\"AP\"]";
    
    if (archipelago_deathlink) {
        json << ",\"slot_data\":{\"death_link\":true}";
    }
    
    json << "}]";
    
    ArchipelagoMessage msg;
    msg.type = ArchipelagoMessageType::DATA;
    msg.data = json.str();
    
    m_socket->SendMessage(msg);
}

void SelacoArchipelago::SendDeathLink(const std::string& cause) {
    if (!m_connected || !archipelago_deathlink) return;
    
    std::stringstream json;
    json << "[{";
    json << "\"cmd\":\"Bounce\",";
    json << "\"data\":{";
    json << "\"type\":\"DeathLink\",";
    json << "\"cause\":\"" << cause << "\",";
    json << "\"source\":\"" << m_slot_name << "\"";
    json << "}";
    json << "}]";
    
    ArchipelagoMessage msg;
    msg.type = ArchipelagoMessageType::DATA;
    msg.data = json.str();
    
    m_socket->SendMessage(msg);
}

void SelacoArchipelago::LoadItemData() {
    // Load all item definitions
    m_items.clear();
    
    for (size_t i = 0; i < ITEM_DEFINITIONS.size(); ++i) {
        const ItemDef& item = ITEM_DEFINITIONS[i];
        m_items[item.id] = item;
    }
    
    Printf("Loaded %zu item definitions\n", m_items.size());
}

void SelacoArchipelago::LoadLocationData() {
    // Load all location definitions
    m_locations.clear();
    
    for (size_t i = 0; i < LOCATION_DEFINITIONS.size(); ++i) {
        const LocationDef& location = LOCATION_DEFINITIONS[i];
        m_locations[location.id] = location;
    }
    
    Printf("Loaded %zu location definitions\n", m_locations.size());
}

std::string SelacoArchipelago::GenerateUUID() {
    // Simple UUID generation
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    for (int i = 0; i < 32; ++i) {
        if (i == 8 || i == 12 || i == 16 || i == 20) {
            ss << "-";
        }
        ss << std::hex << dis(gen);
    }
    
    return ss.str();
}

// Console command handlers
void SelacoArchipelago::CMD_CheckLocation(int location_id) {
    if (!g_archipelago || !g_archipelago->IsConnected()) {
        Printf("Not connected to Archipelago\n");
        return;
    }
    
    g_archipelago->CheckLocation(location_id);
}

void SelacoArchipelago::CMD_ListLocations(const std::string& map_name) {
    if (!g_archipelago) {
        Printf("Archipelago not initialized\n");
        return;
    }
    
    Printf("Locations in %s:\n", map_name.c_str());
    
    for (const auto& pair : g_archipelago->m_locations) {
        const LocationDef& loc = pair.second;
        if (loc.map_name == map_name || map_name.empty()) {
            bool checked = std::find(g_archipelago->m_checked_locations.begin(),
                                   g_archipelago->m_checked_locations.end(), pair.first)
                          != g_archipelago->m_checked_locations.end();
            
            Printf("  %d: %s %s\n", pair.first, loc.name.c_str(), 
                   checked ? TEXTCOLOR_GREEN "[CHECKED]" : "");
        }
    }
}

void SelacoArchipelago::CMD_SendItem(int item_id) {
    if (!g_archipelago || !g_archipelago->IsConnected()) {
        Printf("Not connected to Archipelago\n");
        return;
    }
    
    // Simulate receiving an item for testing
    g_archipelago->ReceiveItem(item_id, 0);
}

// Global initialization functions
void Archipelago_Selaco_Init() {
    if (!g_archipelago) {
        g_archipelago = std::make_unique<SelacoArchipelago>();
        g_archipelago->Initialize();
    }
}

void Archipelago_Selaco_Shutdown() {
    if (g_archipelago) {
        g_archipelago->Shutdown();
        g_archipelago.reset();
    }
}

void Archipelago_Selaco_ProcessMessages() {
    if (g_archipelago) {
        g_archipelago->ProcessMessages();
    }
}

bool Archipelago_Selaco_IsConnected() {
    return g_archipelago && g_archipelago->IsConnected();
}

// Console commands
CCMD(archipelago_check_location) {
    if (argv.argc() < 2) {
        Printf("Usage: archipelago_check_location <location_id>\n");
        return;
    }
    
    int location_id = atoi(argv[1]);
    SelacoArchipelago::CMD_CheckLocation(location_id);
}

CCMD(archipelago_list_locations) {
    std::string map_name;
    if (argv.argc() >= 2) {
        map_name = argv[1];
    }
    
    SelacoArchipelago::CMD_ListLocations(map_name);
}

CCMD(archipelago_send_item) {
    if (argv.argc() < 2) {
        Printf("Usage: archipelago_send_item <item_id>\n");
        return;
    }
    
    int item_id = atoi(argv[1]);
    SelacoArchipelago::CMD_SendItem(item_id);
}

CCMD(archipelago_death_link) {
    if (!g_archipelago || !g_archipelago->IsConnected()) {
        Printf("Not connected to Archipelago\n");
        return;
    }
    
    g_archipelago->SendDeathLink("Killed by console command");
    Printf("Death link sent\n");
}

CCMD(archipelago_enable_upgrade) {
    if (argv.argc() < 2) {
        Printf("Usage: archipelago_enable_upgrade <upgrade_name>\n");
        return;
    }
    
    // This would interface with Selaco's weapon upgrade system
    // to enable the specified upgrade
    Printf("Enabling upgrade: %s\n", argv[1]);
}

} // namespace Archipelago