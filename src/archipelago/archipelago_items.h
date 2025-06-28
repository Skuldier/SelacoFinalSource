#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace Archipelago {

// ItemDef structure - represents an item in the Archipelago system
struct ItemDef {
    int64_t id;
    std::string name;
    std::string description;
    
    // Constructor for easy initialization
    ItemDef(int64_t itemId, const std::string& itemName, const std::string& itemDesc = "")
        : id(itemId), name(itemName), description(itemDesc) {}
        
    // Default constructor
    ItemDef() : id(0), name(""), description("") {}
};

// Location structure - represents a location check in the game
struct LocationDef {
    int64_t id;
    std::string name;
    std::string description;
    
    LocationDef(int64_t locId, const std::string& locName, const std::string& locDesc = "")
        : id(locId), name(locName), description(locDesc) {}
        
    LocationDef() : id(0), name(""), description("") {}
};

// Base ID for Selaco items in Archipelago
const int64_t SELACO_BASE_ID = 0x534C00; // "SL" in hex + offset

// Item definitions for Selaco
const std::vector<ItemDef> SELACO_ITEMS = {
    // Weapons (1000-1999)
    ItemDef(SELACO_BASE_ID + 1000, "Pistol", "Standard issue pistol"),
    ItemDef(SELACO_BASE_ID + 1001, "Shotgun", "Close range weapon"),
    ItemDef(SELACO_BASE_ID + 1002, "Assault Rifle", "Rapid fire weapon"),
    ItemDef(SELACO_BASE_ID + 1003, "Grenade Launcher", "Explosive weapon"),
    ItemDef(SELACO_BASE_ID + 1004, "Rocket Launcher", "Heavy explosive weapon"),
    ItemDef(SELACO_BASE_ID + 1005, "Plasma Rifle", "Energy weapon"),
    ItemDef(SELACO_BASE_ID + 1006, "Rail Gun", "Precision weapon"),
    ItemDef(SELACO_BASE_ID + 1007, "Flame Thrower", "Area denial weapon"),
    ItemDef(SELACO_BASE_ID + 1008, "Lightning Gun", "Electric weapon"),
    ItemDef(SELACO_BASE_ID + 1009, "BFG 9000", "Ultimate weapon"),
    
    // Ammo (2000-2999)
    ItemDef(SELACO_BASE_ID + 2000, "Pistol Ammo", "Ammunition for pistol"),
    ItemDef(SELACO_BASE_ID + 2001, "Shotgun Shells", "Ammunition for shotgun"),
    ItemDef(SELACO_BASE_ID + 2002, "Rifle Rounds", "Ammunition for assault rifle"),
    ItemDef(SELACO_BASE_ID + 2003, "Grenades", "Ammunition for grenade launcher"),
    ItemDef(SELACO_BASE_ID + 2004, "Rockets", "Ammunition for rocket launcher"),
    ItemDef(SELACO_BASE_ID + 2005, "Energy Cells", "Ammunition for plasma rifle"),
    ItemDef(SELACO_BASE_ID + 2006, "Rail Slugs", "Ammunition for rail gun"),
    ItemDef(SELACO_BASE_ID + 2007, "Fuel Canister", "Ammunition for flame thrower"),
    ItemDef(SELACO_BASE_ID + 2008, "Lightning Cores", "Ammunition for lightning gun"),
    ItemDef(SELACO_BASE_ID + 2009, "BFG Cells", "Ammunition for BFG 9000"),
    
    // Health & Armor (3000-3999)
    ItemDef(SELACO_BASE_ID + 3000, "Health Pack Small", "Restores 25 health"),
    ItemDef(SELACO_BASE_ID + 3001, "Health Pack Large", "Restores 50 health"),
    ItemDef(SELACO_BASE_ID + 3002, "Armor Shard", "Small armor boost (+5)"),
    ItemDef(SELACO_BASE_ID + 3003, "Armor Vest", "Medium armor (+50)"),
    ItemDef(SELACO_BASE_ID + 3004, "Full Armor", "Complete armor set (+100)"),
    ItemDef(SELACO_BASE_ID + 3005, "Mega Health", "Maximum health boost (+100)"),
    ItemDef(SELACO_BASE_ID + 3006, "Shield Generator", "Temporary invulnerability"),
    
    // Keys & Access (4000-4999)
    ItemDef(SELACO_BASE_ID + 4000, "Red Keycard", "Opens red security doors"),
    ItemDef(SELACO_BASE_ID + 4001, "Blue Keycard", "Opens blue security doors"),
    ItemDef(SELACO_BASE_ID + 4002, "Yellow Keycard", "Opens yellow security doors"),
    ItemDef(SELACO_BASE_ID + 4003, "Green Keycard", "Opens green security doors"),
    ItemDef(SELACO_BASE_ID + 4004, "Security Pass Alpha", "Level 1 clearance"),
    ItemDef(SELACO_BASE_ID + 4005, "Security Pass Beta", "Level 2 clearance"),
    ItemDef(SELACO_BASE_ID + 4006, "Security Pass Omega", "Maximum clearance"),
    ItemDef(SELACO_BASE_ID + 4007, "Master Key", "Opens all standard locks"),
    
    // Upgrades & Abilities (5000-5999)
    ItemDef(SELACO_BASE_ID + 5000, "Damage Boost", "Increases weapon damage by 25%"),
    ItemDef(SELACO_BASE_ID + 5001, "Speed Boost", "Increases movement speed by 20%"),
    ItemDef(SELACO_BASE_ID + 5002, "Jump Boots", "Enhanced jumping ability"),
    ItemDef(SELACO_BASE_ID + 5003, "Night Vision", "See in the dark"),
    ItemDef(SELACO_BASE_ID + 5004, "Radar Scanner", "Minimap enemy detection"),
    ItemDef(SELACO_BASE_ID + 5005, "Stealth Module", "Reduced enemy detection"),
    ItemDef(SELACO_BASE_ID + 5006, "Ammo Converter", "Universal ammo pickup"),
    ItemDef(SELACO_BASE_ID + 5007, "Health Regenerator", "Slow health regeneration"),
    
    // Special Items (6000-6999)
    ItemDef(SELACO_BASE_ID + 6000, "Map Revealer", "Reveals level layout"),
    ItemDef(SELACO_BASE_ID + 6001, "Secret Detector", "Highlights secret areas"),
    ItemDef(SELACO_BASE_ID + 6002, "Checkpoint Token", "Allows saving progress"),
    ItemDef(SELACO_BASE_ID + 6003, "Backpack", "Increases ammo capacity"),
    ItemDef(SELACO_BASE_ID + 6004, "Berserk Pack", "Melee damage boost"),
    ItemDef(SELACO_BASE_ID + 6005, "Time Slower", "Bullet time effect"),
    
    // Progressive Items (7000-7999)
    ItemDef(SELACO_BASE_ID + 7000, "Progressive Weapon", "Unlocks next weapon tier"),
    ItemDef(SELACO_BASE_ID + 7001, "Progressive Armor", "Unlocks next armor tier"),
    ItemDef(SELACO_BASE_ID + 7002, "Progressive Access", "Unlocks next security level"),
    ItemDef(SELACO_BASE_ID + 7003, "Progressive Ability", "Unlocks next ability")
};

// Location definitions for Selaco
const std::vector<LocationDef> SELACO_LOCATIONS = {
    // Level completion locations
    LocationDef(SELACO_BASE_ID + 10000, "Complete Chapter 1", "Finish the first chapter"),
    LocationDef(SELACO_BASE_ID + 10001, "Complete Chapter 2", "Finish the second chapter"),
    LocationDef(SELACO_BASE_ID + 10002, "Complete Chapter 3", "Finish the third chapter"),
    
    // Secret locations
    LocationDef(SELACO_BASE_ID + 11000, "Chapter 1 Secret 1", "Find secret area in chapter 1"),
    LocationDef(SELACO_BASE_ID + 11001, "Chapter 1 Secret 2", "Find hidden room in chapter 1"),
    
    // Boss locations
    LocationDef(SELACO_BASE_ID + 12000, "Defeat Security Chief", "Defeat the first boss"),
    LocationDef(SELACO_BASE_ID + 12001, "Defeat Mech Guardian", "Defeat the second boss"),
    
    // Collection locations
    LocationDef(SELACO_BASE_ID + 13000, "Collect 10 Data Logs", "Find 10 story logs"),
    LocationDef(SELACO_BASE_ID + 13001, "Collect All Weapons", "Find every weapon type")
};

// Helper functions for item management
inline const ItemDef* GetItemById(int64_t id) {
    for (const auto& item : SELACO_ITEMS) {
        if (item.id == id) {
            return &item;
        }
    }
    return nullptr;
}

inline const ItemDef* GetItemByName(const std::string& name) {
    for (const auto& item : SELACO_ITEMS) {
        if (item.name == name) {
            return &item;
        }
    }
    return nullptr;
}

inline const LocationDef* GetLocationById(int64_t id) {
    for (const auto& loc : SELACO_LOCATIONS) {
        if (loc.id == id) {
            return &loc;
        }
    }
    return nullptr;
}

inline const LocationDef* GetLocationByName(const std::string& name) {
    for (const auto& loc : SELACO_LOCATIONS) {
        if (loc.name == name) {
            return &loc;
        }
    }
    return nullptr;
}

} // namespace Archipelago