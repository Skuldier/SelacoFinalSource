#pragma once

// Forward declarations
class FSerializer;
struct player_t;

// Main item processing function
void ProcessArchipelagoItem(int itemId, const char* itemName);

// Serialization support
void SerializeArchipelagoState(FSerializer& arc);

// Item ID ranges for Selaco
enum ArchipelagoItemRanges {
    // Weapons
    ITEM_WEAPON_START = 50000,
    ITEM_WEAPON_END = 50009,
    
    // Ammo
    ITEM_AMMO_START = 50010,
    ITEM_AMMO_END = 50019,
    
    // Health & Armor
    ITEM_HEALTH_START = 50020,
    ITEM_HEALTH_END = 50026,
    
    // Keys
    ITEM_KEY_START = 50030,
    ITEM_KEY_END = 50037,
    
    // Powerups & Upgrades
    ITEM_POWERUP_START = 50040,
    ITEM_POWERUP_END = 50047,
    
    // Special Items
    ITEM_SPECIAL_START = 50050,
    ITEM_SPECIAL_END = 50055,
    
    // Progressive Items
    ITEM_PROGRESSIVE_START = 50060,
    ITEM_PROGRESSIVE_END = 50063
};

// Console command for testing
// Usage: archipelago_testitem <item_id> [item_name]