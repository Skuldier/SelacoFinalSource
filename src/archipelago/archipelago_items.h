#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace Archipelago {

// Item categories for organization
enum class ItemCategory {
    PROGRESSION,        // Required for game progression
    WEAPON,            // Weapon pickups
    WEAPON_UPGRADE,    // Weapon modifications
    EQUIPMENT,         // Throwables and tools
    HEALTH,            // Health pickups
    ARMOR,             // Armor pickups
    AMMO,              // Ammunition
    COLLECTIBLE,       // Optional collectibles
    CONSUMABLE,        // Credits, weapon parts, etc.
    UPGRADE            // Player upgrades
};

// Item flags
enum class ItemFlag {
    NONE = 0,
    ESSENTIAL = 1 << 0,      // Cannot be randomized away
    UPGRADE_ITEM = 1 << 1,   // Is an upgrade
    PROGRESSION = 1 << 2,    // Required for progression
    FILLER = 1 << 3,         // Can be used as filler
    TRAP = 1 << 4            // Trap item (future use)
};

// Item definition structure
struct ItemDef {
    int id;
    std::string name;
    std::string internal_name;
    ItemCategory category;
    int flags;
    int max_quantity;
};

// Forward declaration - full definition is in archipelago_locations.h
struct LocationDef;

// Item definitions
const std::vector<ItemDef> ITEM_DEFINITIONS = {
    // ===== PROGRESSION ITEMS =====
    {1001, "Purple Keycard", "OMNI_PURPLECARD", ItemCategory::PROGRESSION, 
     static_cast<int>(ItemFlag::ESSENTIAL) | static_cast<int>(ItemFlag::PROGRESSION), 1},
    {1002, "Yellow Keycard", "OMNI_YELLOWCARD", ItemCategory::PROGRESSION,
     static_cast<int>(ItemFlag::ESSENTIAL) | static_cast<int>(ItemFlag::PROGRESSION), 1},
    {1003, "Blue Keycard", "OMNI_BLUECARD", ItemCategory::PROGRESSION,
     static_cast<int>(ItemFlag::ESSENTIAL) | static_cast<int>(ItemFlag::PROGRESSION), 1},
    {1004, "Security Clearance Upgrade", "SecurityCard", ItemCategory::PROGRESSION,
     static_cast<int>(ItemFlag::ESSENTIAL) | static_cast<int>(ItemFlag::PROGRESSION) | static_cast<int>(ItemFlag::UPGRADE_ITEM), 7},
    {1005, "Cabinet Keycard", "CabinetCard", ItemCategory::PROGRESSION,
     static_cast<int>(ItemFlag::ESSENTIAL), 6},
    {1006, "Demolition Charges", "DEMOLITIONCHARGE_PICKUP", ItemCategory::PROGRESSION,
     static_cast<int>(ItemFlag::ESSENTIAL), 1},

    // ===== WEAPON PICKUPS =====
    {2001, "Roaring Cricket", "RoaringCricketPickup", ItemCategory::WEAPON, 0, 1},
    {2002, "Shotgun", "ShotgunPickup", ItemCategory::WEAPON, 0, 1},
    {2003, "Assault Rifle", "RiflePickup", ItemCategory::WEAPON, 0, 1},
    {2004, "SMG", "SMGPickup", ItemCategory::WEAPON, 0, 1},
    {2005, "Grenade Launcher", "GrenadeLauncherPickup", ItemCategory::WEAPON, 0, 1},
    {2006, "Nailgun", "NailgunPickup", ItemCategory::WEAPON, 0, 1},
    {2007, "Plasma Rifle", "PlasmaRiflePickup", ItemCategory::WEAPON, 0, 1},
    {2008, "DMR", "DMRPickup", ItemCategory::WEAPON, 0, 1},
    {2009, "Railgun", "RailgunPickup", ItemCategory::WEAPON, 0, 1},

    // ===== WEAPON UPGRADES =====
    // Cricket Upgrades
    {3001, "Cricket: Elephant Rounds", "UpgradeCricketElephant", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3002, "Cricket: Knockback", "UpgradeCricketKnockback", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3003, "Cricket: Hair Trigger", "UpgradeCricketHairTrigger", ItemCategory::WEAPON_UPGRADE,
     static_cast<int>(ItemFlag::UPGRADE_ITEM), 1},
    {3004, "Cricket: Headshot Damage", "UpgradeCricketHeadshotDamage", ItemCategory::WEAPON_UPGRADE,
     static_cast<int>(ItemFlag::UPGRADE_ITEM), 1},
    {3005, "Cricket: Recoil Dampener", "UpgradeCricketRecoil", ItemCategory::WEAPON_UPGRADE,
     static_cast<int>(ItemFlag::UPGRADE_ITEM), 1},
    {3006, "Cricket: Impact", "UpgradeCricketImpact", ItemCategory::WEAPON_UPGRADE,
     static_cast<int>(ItemFlag::UPGRADE_ITEM), 1},
    {3007, "Cricket: Splash Damage", "UpgradeCricketSplashDamage", ItemCategory::WEAPON_UPGRADE,
     static_cast<int>(ItemFlag::UPGRADE_ITEM), 1},

    // Shotgun Upgrades
    {3010, "Shotgun: Incendiary Shells", "UpgradeShotgunIncendiary", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3011, "Shotgun: Choke", "UpgradeShotgunChoke", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3012, "Shotgun: Crowd Control", "UpgradeShotgunCrowdControl", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3013, "Shotgun: Gore Mode", "UpgradeShotgunGore", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3014, "Shotgun: Extra Pellets", "UpgradeShotgunPellets", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},

    // Rifle Upgrades
    {3020, "Rifle: Armor Piercing", "UpgradeRifleArmorPiercing", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3021, "Rifle: Extended Magazine", "UpgradeRifleExtendedMag", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3022, "Rifle: Stabilizer", "UpgradeRifleStabilizer", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},

    // SMG Upgrades
    {3030, "SMG: Extended Magazine", "UpgradeSMGExtendedMag", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3031, "SMG: Dual Wield", "UpgradeSMGDualWield", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3032, "SMG: Incendiary Rounds", "UpgradeSMGIncendiary", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},

    // Grenade Launcher Upgrades
    {3040, "GL: Remote Detonation", "UpgradeGLRemoteDetonation", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3041, "GL: Cluster Bombs", "UpgradeGLClusterBombs", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3042, "GL: Arc Trajectory", "UpgradeGLArcTrajectory", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},

    // Nailgun Upgrades
    {3050, "Nailgun: Incendiary Nails", "UpgradeNailgunIncendiary", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3051, "Nailgun: Expanded Ammo", "UpgradeNailgunExpandedAmmo", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3052, "Nailgun: Ricochet", "UpgradeNailgunRicochet", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},

    // Plasma Rifle Upgrades
    {3060, "Plasma: Overcharge", "UpgradePlasmaOvercharge", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3061, "Plasma: Overcapacity", "UpgradePlasmaRifleOvercapacity", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3062, "Plasma: Chain Lightning", "UpgradePlasmaChainLightning", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},

    // DMR Upgrades  
    {3070, "DMR: Silencer", "UpgradeDMRSilencer", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3071, "DMR: Extended Magazine", "UpgradeDMRExtendedMagazine", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3072, "DMR: Bipod", "UpgradeDMRBipod", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3073, "DMR: Headshot Multiplier", "UpgradeDMRHeadshot", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},

    // Grenade Upgrades
    {3080, "Grenade: Increased Radius", "UpgradeGrenadeRadius", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3081, "Grenade: Damage Boost", "UpgradeGrenadeDamage", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3082, "Grenade: Self Damage Reduction", "UpgradeGrenadeSelfDamage", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3083, "Grenade: Stealth", "UpgradeGrenadeStealth", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},

    // Mine Upgrades
    {3090, "Mine: Damage Boost", "UpgradeMineDamage", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3091, "Mine: Resource Extractor", "UpgradeMineExtractor", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3092, "Mine: Increased Radius", "UpgradeMineRadius", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3093, "Mine: Face Hopper", "UpgradeMineFaceHopper", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},

    // Weapon Alt-Fire Modes
    {3100, "Grenade: Impact Mode", "AltFireGrenadeImpact", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3101, "Grenade: Sticky Mode", "AltFireGrenadeSticky", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3102, "Grenade: Balloon Mode", "AltFireGrenadeBalloon", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3103, "Mine: Second Charge", "AltFireMineSecondCharge", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3104, "Mine: Shock Mode", "AltFireMineShock", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},
    {3105, "Mine: Annihilation Mode", "AltFireMineAnnihilation", ItemCategory::WEAPON_UPGRADE,
     (int)ItemFlag::UPGRADE_ITEM, 1},

    // ===== EQUIPMENT =====
    {4001, "Hand Grenade", "HandGrenadeAmmo", ItemCategory::EQUIPMENT, 0, 3},
    {4002, "Ice Grenade", "IceGrenadeAmmo", ItemCategory::EQUIPMENT, 0, 3},
    {4003, "Mine", "MineAmmo", ItemCategory::EQUIPMENT, 0, 2},
    {4004, "Grenade Bandolier", "GrenadeBandolier", ItemCategory::EQUIPMENT,
     (int)ItemFlag::ESSENTIAL | (int)ItemFlag::UPGRADE_ITEM, 1},

    // ===== HEALTH ITEMS =====
    {5001, "Small Health Pack", "Stimpack", ItemCategory::HEALTH, (int)ItemFlag::FILLER, 999},
    {5002, "Large Health Pack", "Medkit", ItemCategory::HEALTH, (int)ItemFlag::FILLER, 999},
    {5003, "Ultra Health", "UltraHealth", ItemCategory::HEALTH, 0, 999},
    {5004, "Portable Medkit", "PortableMedkit", ItemCategory::HEALTH, 0, 3},
    {5005, "Health Upgrade", "HealthUpgrade", ItemCategory::UPGRADE,
     (int)ItemFlag::ESSENTIAL | (int)ItemFlag::UPGRADE_ITEM, 10},

    // ===== ARMOR ITEMS =====
    {6001, "Light Armor", "CommonArmor", ItemCategory::ARMOR, (int)ItemFlag::FILLER, 999},
    {6002, "Combat Armor", "RareArmor", ItemCategory::ARMOR, (int)ItemFlag::FILLER, 999},
    {6003, "Heavy Armor", "EpicArmor", ItemCategory::ARMOR, 0, 999},
    {6004, "Admiral Armor", "LegendaryArmor", ItemCategory::ARMOR, 0, 999},
    {6005, "Armor Shard", "ArmorShardPickup", ItemCategory::ARMOR, (int)ItemFlag::FILLER, 999},

    // ===== AMMO PICKUPS =====
    {7001, "Pistol Ammo", "AmmoPistolSmall", ItemCategory::AMMO, (int)ItemFlag::FILLER, 999},
    {7002, "Shotgun Shells (Small)", "AmmoShotgunSmall", ItemCategory::AMMO, (int)ItemFlag::FILLER, 999},
    {7003, "Shotgun Shells (Medium)", "AmmoShotgunMedium", ItemCategory::AMMO, (int)ItemFlag::FILLER, 999},
    {7004, "Shotgun Shells (Large)", "AmmoShotgunLarge", ItemCategory::AMMO, (int)ItemFlag::FILLER, 999},
    {7005, "Rifle Ammo (Small)", "AmmoRifleSmall", ItemCategory::AMMO, (int)ItemFlag::FILLER, 999},
    {7006, "Rifle Ammo (Medium)", "AmmoRifleMedium", ItemCategory::AMMO, (int)ItemFlag::FILLER, 999},
    {7007, "Rifle Ammo (Large)", "AmmoRifleLarge", ItemCategory::AMMO, (int)ItemFlag::FILLER, 999},
    {7008, "Grenade Ammo", "AmmoGrenadeSmall", ItemCategory::AMMO, (int)ItemFlag::FILLER, 999},
    {7009, "Nailgun Ammo (Small)", "AmmoNailgunSmall", ItemCategory::AMMO, (int)ItemFlag::FILLER, 999},
    {7010, "Nailgun Ammo (Medium)", "AmmoNailgunMedium", ItemCategory::AMMO, (int)ItemFlag::FILLER, 999},
    {7011, "DMR Ammo", "AmmoSniperMedium", ItemCategory::AMMO, (int)ItemFlag::FILLER, 999},
    {7012, "Plasma Ammo (Small)", "AmmoEnergySmall", ItemCategory::AMMO, (int)ItemFlag::FILLER, 999},
    {7013, "Plasma Ammo (Large)", "AmmoEnergyLarge", ItemCategory::AMMO, (int)ItemFlag::FILLER, 999},
    {7014, "Railgun Slug", "AmmoRailgunSmall", ItemCategory::AMMO, (int)ItemFlag::FILLER, 999},
    {7015, "Acid Ammo", "AmmoGLAcid", ItemCategory::AMMO, (int)ItemFlag::FILLER, 999},

    // ===== COLLECTIBLES =====
    {8001, "Trading Card", "TradingCard", ItemCategory::COLLECTIBLE, 0, 999},
    {8002, "PDA Entry", "PDAEntry", ItemCategory::COLLECTIBLE, 0, 999},

    // ===== CONSUMABLES =====
    {9001, "Credits (Small)", "CreditsSmall", ItemCategory::CONSUMABLE, (int)ItemFlag::FILLER, 999},
    {9002, "Credits (Medium)", "CreditsbagMedium", ItemCategory::CONSUMABLE, (int)ItemFlag::FILLER, 999},
    {9003, "Credits (Large)", "CreditsbagLarge", ItemCategory::CONSUMABLE, (int)ItemFlag::FILLER, 999},
    {9004, "Weapon Parts", "WeaponPartPickup", ItemCategory::CONSUMABLE, (int)ItemFlag::FILLER, 999},
    {9005, "Weapon Capacity Kit", "WeaponCapacityKit", ItemCategory::UPGRADE,
     (int)ItemFlag::ESSENTIAL | (int)ItemFlag::UPGRADE_ITEM, 20},
    {9006, "Workshop Tech Module", "TechModule", ItemCategory::UPGRADE,
     (int)ItemFlag::ESSENTIAL | (int)ItemFlag::UPGRADE_ITEM, 6}
};

// Map definitions
const std::vector<std::string> MAP_LIST = {
    "SE_01A",  // Pathfinder Hospital
    "SE_01B",  // Pathfinder Hospital (Blue)
    "SE_01C",  // Pathfinder Labs
    "SE_02A",  // Utility Area
    "SE_02Z",  // Pathfinder Hospital (Orange)
    "SE_02B",  // Utility Area
    "SE_02C",  // Water Treatment
    "SE_03A",  // Parking Garage
    "SE_03A1", // Parking Garage
    "SE_03B",  // Selaco Streets
    "SE_03B1", // Selaco Streets
    "SE_03B2", // Sal's Bar
    "SE_03C",  // Sal's Lair
    "SE_04A",  // Office Complex
    "SE_04B",  // Administration
    "SE_04C",  // Courtyard
    "SE_05A",  // Exodus Plaza
    "SE_05B",  // Exodus Plaza - North
    "SE_05C",  // Exodus Plaza - South
    "SE_05D",  // Exodus Plaza - Front Entrance
    "SE_06A",  // Plant Cloning Facility - Offices
    "SE_06A1", // Plant Cloning Facility - Offices
    "SE_06B",  // Plant Cloning Facility - Research Labs
    "SE_06C",  // Plant Cloning Facility - Cloning Plant
    "SE_07A1", // Starlight Exterior
    "SE_07A",  // Starlight Lobby
    "SE_07B",  // Starlight Green
    "SE_07C",  // Starlight Red
    "SE_07D",  // Starlight Blue
    "SE_07E",  // Starlight Purple
    "SE_07Z",  // Starlight Purple
    "SE_08A",  // Endgame
    "SE_SAFE"  // Safe Room Extension
};

// Location categories
enum class LocationCategory {
    ITEM_PICKUP,      // Standard item spawn
    CABINET,          // Cabinet that requires keycard
    SECRET,           // Secret area
    SHOP,             // Workshop purchase
    BOSS_REWARD,      // Boss kill reward
    OBJECTIVE         // Objective completion
};

// Helper functions
inline bool HasFlag(const ItemDef& item, ItemFlag flag) {
    return (item.flags & static_cast<int>(flag)) != 0;
}

inline bool IsProgressionItem(const ItemDef& item) {
    return HasFlag(item, ItemFlag::PROGRESSION);
}

inline bool IsEssentialItem(const ItemDef& item) {
    return HasFlag(item, ItemFlag::ESSENTIAL);
}

inline bool IsFillerItem(const ItemDef& item) {
    return HasFlag(item, ItemFlag::FILLER);
}

// Item ID ranges for categorization
constexpr int PROGRESSION_START = 1000;
constexpr int WEAPON_START = 2000;
constexpr int UPGRADE_START = 3000;
constexpr int EQUIPMENT_START = 4000;
constexpr int HEALTH_START = 5000;
constexpr int ARMOR_START = 6000;
constexpr int AMMO_START = 7000;
constexpr int COLLECTIBLE_START = 8000;
constexpr int CONSUMABLE_START = 9000;

} // namespace Archipelago