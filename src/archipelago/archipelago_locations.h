#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include "archipelago_items.h"  // Need full definition of LocationCategory

namespace Archipelago {

// Location definition with requirements
struct LocationDef {
    int id;
    std::string name;
    std::string map_name;
    LocationCategory category;
    bool is_secret;
    
    // Requirements
    int required_clearance_level;  // 0 = no clearance required
    std::vector<int> required_items;  // Item IDs required to access
    
    // For cabinet locations
    int cabinet_keycard_cost;  // 0 = not a cabinet
    
    // Original item (for vanilla placement tracking)
    int original_item_id;
};

// Location ID ranges by map
constexpr int LOC_SE_01A_START = 10000;
constexpr int LOC_SE_01B_START = 11000;
constexpr int LOC_SE_01C_START = 12000;
constexpr int LOC_SE_02A_START = 13000;
constexpr int LOC_SE_02Z_START = 14000;
constexpr int LOC_SE_02B_START = 15000;
constexpr int LOC_SE_02C_START = 16000;
constexpr int LOC_SE_03A_START = 17000;
constexpr int LOC_SE_03A1_START = 18000;
constexpr int LOC_SE_03B_START = 19000;
constexpr int LOC_SE_03B1_START = 20000;
constexpr int LOC_SE_03B2_START = 21000;
constexpr int LOC_SE_03C_START = 22000;
constexpr int LOC_SE_04A_START = 23000;
constexpr int LOC_SE_04B_START = 24000;
constexpr int LOC_SE_04C_START = 25000;
constexpr int LOC_SE_05A_START = 26000;
constexpr int LOC_SE_05B_START = 27000;
constexpr int LOC_SE_05C_START = 28000;
constexpr int LOC_SE_05D_START = 29000;
constexpr int LOC_SE_06A_START = 30000;
constexpr int LOC_SE_06A1_START = 31000;
constexpr int LOC_SE_06B_START = 32000;
constexpr int LOC_SE_06C_START = 33000;
constexpr int LOC_SE_07A1_START = 34000;
constexpr int LOC_SE_07A_START = 35000;
constexpr int LOC_SE_07B_START = 36000;
constexpr int LOC_SE_07C_START = 37000;
constexpr int LOC_SE_07D_START = 38000;
constexpr int LOC_SE_07E_START = 39000;
constexpr int LOC_SE_07Z_START = 40000;
constexpr int LOC_SE_08A_START = 41000;
constexpr int LOC_SE_SAFE_START = 42000;

// Example location definitions for SE_01A (Pathfinder Hospital)
// In a real implementation, these would be generated from map analysis
inline const std::vector<LocationDef>& GetLocationDefinitions() {
    static const std::vector<LocationDef> LOCATION_DEFINITIONS = {
    // ===== SE_01A: Pathfinder Hospital =====
    // Main path items
    {10001, "Hospital Entrance - Health Pack", "SE_01A", LocationCategory::ITEM_PICKUP, 
     false, 0, {}, 0, 5001},
    {10002, "Hospital Lobby - Pistol Ammo", "SE_01A", LocationCategory::ITEM_PICKUP,
     false, 0, {}, 0, 7001},
    {10003, "Hospital Corridor - Shotgun", "SE_01A", LocationCategory::ITEM_PICKUP,
     false, 0, {}, 0, 2002},
    {10004, "Hospital Medical Wing - Medkit", "SE_01A", LocationCategory::ITEM_PICKUP,
     false, 0, {}, 0, 5002},
    {10005, "Hospital Security - Purple Keycard", "SE_01A", LocationCategory::ITEM_PICKUP,
     false, 0, {}, 0, 1001},
    
    // Cabinet locations
    {10101, "Hospital Cabinet 1", "SE_01A", LocationCategory::CABINET,
     false, 0, {}, 1, 0},  // Costs 1 cabinet card
    {10102, "Hospital Cabinet 2", "SE_01A", LocationCategory::CABINET,
     false, 0, {}, 1, 0},
    {10103, "Hospital Cabinet 3", "SE_01A", LocationCategory::CABINET,
     false, 0, {}, 1, 0},
    
    // Secret locations
    {10201, "Hospital Secret Room - Trading Card", "SE_01A", LocationCategory::SECRET,
     true, 0, {}, 0, 8001},
    {10202, "Hospital Vent Secret - Armor", "SE_01A", LocationCategory::SECRET,
     true, 0, {}, 0, 6001},
    {10203, "Hospital Hidden Stash - Credits", "SE_01A", LocationCategory::SECRET,
     true, 0, {}, 0, 9002},
    
    // Purple keycard locked areas
    {10301, "Hospital Lab - Health Upgrade", "SE_01A", LocationCategory::ITEM_PICKUP,
     false, 0, {1001}, 0, 5005},  // Requires purple keycard
    {10302, "Hospital Lab - Tech Module", "SE_01A", LocationCategory::ITEM_PICKUP,
     false, 0, {1001}, 0, 9006},
    
    // ===== SE_01B: Pathfinder Hospital (Blue) =====
    {11001, "Blue Wing Entrance - Rifle Ammo", "SE_01B", LocationCategory::ITEM_PICKUP,
     false, 0, {}, 0, 7005},
    {11002, "Blue Wing Storage - Weapon Parts", "SE_01B", LocationCategory::ITEM_PICKUP,
     false, 0, {}, 0, 9004},
    {11003, "Blue Wing Office - Yellow Keycard", "SE_01B", LocationCategory::ITEM_PICKUP,
     false, 0, {}, 0, 1002},
    {11004, "Blue Wing Armory - Assault Rifle", "SE_01B", LocationCategory::ITEM_PICKUP,
     false, 0, {1002}, 0, 2003},  // Requires yellow keycard
    
    // ===== SE_01C: Pathfinder Labs =====
    {12001, "Labs Reception - Nailgun", "SE_01C", LocationCategory::ITEM_PICKUP,
     false, 0, {}, 0, 2006},
    {12002, "Labs Research - Blue Keycard", "SE_01C", LocationCategory::ITEM_PICKUP,
     false, 0, {}, 0, 1003},
    {12003, "Labs Clearance Upgrade", "SE_01C", LocationCategory::ITEM_PICKUP,
     false, 0, {1003}, 0, 1004},  // Requires blue keycard
    
    // ===== Workshop/Shop Locations =====
    {50001, "Workshop - Cricket Hair Trigger", "SE_SAFE", LocationCategory::SHOP,
     false, 0, {}, 0, 3003},
    {50002, "Workshop - Shotgun Choke", "SE_SAFE", LocationCategory::SHOP,
     false, 0, {}, 0, 3011},
    {50003, "Workshop - Rifle Extended Mag", "SE_SAFE", LocationCategory::SHOP,
     false, 0, {}, 0, 3021}
};
    return LOCATION_DEFINITIONS;
}

// Helper structure for tracking location availability
struct LocationAccess {
    bool accessible;
    std::vector<int> missing_items;
    int missing_clearance;
};

// Check if a location is accessible with current items
inline LocationAccess CheckLocationAccess(const LocationDef& location, 
                                         const std::vector<int>& owned_items,
                                         int clearance_level) {
    LocationAccess access;
    access.accessible = true;
    access.missing_clearance = 0;
    
    // Check clearance requirement
    if (location.required_clearance_level > clearance_level) {
        access.accessible = false;
        access.missing_clearance = location.required_clearance_level - clearance_level;
    }
    
    // Check item requirements
    for (size_t i = 0; i < location.required_items.size(); ++i) {
        int required_item = location.required_items[i];
        if (std::find(owned_items.begin(), owned_items.end(), required_item) == owned_items.end()) {
            access.accessible = false;
            access.missing_items.push_back(required_item);
        }
    }
    
    return access;
}

// Get all accessible locations
inline std::vector<const LocationDef*> GetAccessibleLocations(
    const std::vector<int>& owned_items,
    int clearance_level,
    int cabinet_cards = 0) {
    
    const auto& all_locations = GetLocationDefinitions();
    std::vector<const LocationDef*> accessible;
    
    for (size_t i = 0; i < all_locations.size(); ++i) {
        const LocationDef& location = all_locations[i];
        auto access = CheckLocationAccess(location, owned_items, clearance_level);
        
        // Special check for cabinets
        if (location.category == LocationCategory::CABINET) {
            if (cabinet_cards >= location.cabinet_keycard_cost && access.accessible) {
                accessible.push_back(&location);
            }
        } else if (access.accessible) {
            accessible.push_back(&location);
        }
    }
    
    return accessible;
}

// Get locations by map
inline std::vector<const LocationDef*> GetLocationsByMap(
    const std::string& map_name) {
    
    const auto& all_locations = GetLocationDefinitions();
    std::vector<const LocationDef*> map_locations;
    
    for (size_t i = 0; i < all_locations.size(); ++i) {
        const LocationDef& location = all_locations[i];
        if (location.map_name == map_name) {
            map_locations.push_back(&location);
        }
    }
    
    return map_locations;
}

// Logic rules for progression
struct ProgressionRule {
    std::string description;
    std::vector<int> required_items;
    std::vector<int> unlocks_items;
};

// Example progression rules
inline const std::vector<ProgressionRule>& GetProgressionRules() {
    static const std::vector<ProgressionRule> PROGRESSION_RULES = {
        {"Access Blue Wing", {1001}, {}},  // Purple keycard opens blue wing
        {"Access Yellow Areas", {1002}, {}},  // Yellow keycard
        {"Access Blue Areas", {1003}, {}},  // Blue keycard
        {"Open Level 1 Doors", {1004}, {}},  // Security clearance level 1
        {"Access Cabinets", {1005}, {}},  // Cabinet keycards
        {"Destroy Barriers", {1006}, {}},  // Demolition charges
    };
    return PROGRESSION_RULES;
}

} // namespace Archipelago