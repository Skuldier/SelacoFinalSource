#include "archipelago_items.h"
#include "archipelago_integration.h"
#include "archipelago_socket.h"
#include "doomtype.h"
#include "c_dispatch.h"
#include "c_cvars.h"
#include "g_game.h"
#include "p_local.h"
#include "gi.h"
#include "a_pickups.h"
#include "a_weapons.h"
#include "a_keys.h"
#include "p_acs.h"

using namespace Archipelago;

// External references
extern std::unique_ptr<ArchipelagoSocket> g_archipelagoSocket;
EXTERN_CVAR(Bool, archipelago_debug)

// Track received items to prevent duplicates
static std::set<int64_t> g_receivedItems;
static std::set<int64_t> g_checkedLocations;

// Implementation of Archipelago event handlers
namespace ArchipelagoEvents {

void OnItemReceived(int itemId, const char* itemName) {
    // Prevent duplicate item grants
    if (g_receivedItems.find(itemId) != g_receivedItems.end()) {
        if (archipelago_debug) {
            Printf("Item %d already received, skipping\n", itemId);
        }
        return;
    }
    
    g_receivedItems.insert(itemId);
    
    const ItemDef* item = GetItemById(itemId);
    if (!item) {
        Printf(TEXTCOLOR_RED "Unknown item ID: %d\n", itemId);
        return;
    }
    
    Printf(TEXTCOLOR_GREEN "Archipelago: Received %s\n", item->name.c_str());
    
    // Get the current player
    auto player = players[consoleplayer].mo;
    if (!player) {
        Printf(TEXTCOLOR_RED "Error: No player actor found\n");
        return;
    }
    
    // Grant items based on ID ranges
    int64_t baseId = itemId - SELACO_BASE_ID;
    
    // Weapons (1000-1999)
    if (baseId >= 1000 && baseId < 2000) {
        switch (baseId) {
            case 1000: GiveWeaponToPlayer(player, "Pistol"); break;
            case 1001: GiveWeaponToPlayer(player, "Shotgun"); break;
            case 1002: GiveWeaponToPlayer(player, "AssaultRifle"); break;
            case 1003: GiveWeaponToPlayer(player, "GrenadeLauncher"); break;
            case 1004: GiveWeaponToPlayer(player, "RocketLauncher"); break;
            case 1005: GiveWeaponToPlayer(player, "PlasmaRifle"); break;
            case 1006: GiveWeaponToPlayer(player, "RailGun"); break;
            case 1007: GiveWeaponToPlayer(player, "FlameThrower"); break;
            case 1008: GiveWeaponToPlayer(player, "LightningGun"); break;
            case 1009: GiveWeaponToPlayer(player, "BFG9000"); break;
            default:
                Printf(TEXTCOLOR_YELLOW "Weapon %s not implemented\n", item->name.c_str());
        }
    }
    // Ammo (2000-2999)
    else if (baseId >= 2000 && baseId < 3000) {
        switch (baseId) {
            case 2000: GiveAmmoToPlayer(player, "Clip", 20); break;
            case 2001: GiveAmmoToPlayer(player, "Shell", 8); break;
            case 2002: GiveAmmoToPlayer(player, "RifleAmmo", 30); break;
            case 2003: GiveAmmoToPlayer(player, "GrenadeAmmo", 5); break;
            case 2004: GiveAmmoToPlayer(player, "RocketAmmo", 2); break;
            case 2005: GiveAmmoToPlayer(player, "Cell", 40); break;
            case 2006: GiveAmmoToPlayer(player, "RailAmmo", 10); break;
            case 2007: GiveAmmoToPlayer(player, "FuelAmmo", 50); break;
            case 2008: GiveAmmoToPlayer(player, "LightningAmmo", 20); break;
            case 2009: GiveAmmoToPlayer(player, "BFGAmmo", 5); break;
            default:
                Printf(TEXTCOLOR_YELLOW "Ammo %s not implemented\n", item->name.c_str());
        }
    }
    // Health & Armor (3000-3999)
    else if (baseId >= 3000 && baseId < 4000) {
        switch (baseId) {
            case 3000: GiveHealthToPlayer(player, 25); break;
            case 3001: GiveHealthToPlayer(player, 50); break;
            case 3002: GiveArmorToPlayer(player, 5); break;
            case 3003: GiveArmorToPlayer(player, 50); break;
            case 3004: GiveArmorToPlayer(player, 100); break;
            case 3005: GiveMegaHealthToPlayer(player); break;
            case 3006: GiveInvulnerabilityToPlayer(player, 30); break;
            default:
                Printf(TEXTCOLOR_YELLOW "Health/Armor %s not implemented\n", item->name.c_str());
        }
    }
    // Keys & Access (4000-4999)
    else if (baseId >= 4000 && baseId < 5000) {
        switch (baseId) {
            case 4000: GiveKeyToPlayer(player, "RedCard"); break;
            case 4001: GiveKeyToPlayer(player, "BlueCard"); break;
            case 4002: GiveKeyToPlayer(player, "YellowCard"); break;
            case 4003: GiveKeyToPlayer(player, "GreenCard"); break;
            case 4004: GiveKeyToPlayer(player, "SecurityAlpha"); break;
            case 4005: GiveKeyToPlayer(player, "SecurityBeta"); break;
            case 4006: GiveKeyToPlayer(player, "SecurityOmega"); break;
            case 4007: GiveKeyToPlayer(player, "MasterKey"); break;
            default:
                Printf(TEXTCOLOR_YELLOW "Key %s not implemented\n", item->name.c_str());
        }
    }
    // Upgrades & Abilities (5000-5999)
    else if (baseId >= 5000 && baseId < 6000) {
        switch (baseId) {
            case 5000: ApplyDamageBoost(player, 1.25f); break;
            case 5001: ApplySpeedBoost(player, 1.20f); break;
            case 5002: EnableJumpBoots(player); break;
            case 5003: GivePowerupToPlayer(player, "PowerLightAmp"); break;
            case 5004: EnableRadarScanner(player); break;
            case 5005: GivePowerupToPlayer(player, "PowerInvisibility"); break;
            case 5006: EnableAmmoConverter(player); break;
            case 5007: EnableHealthRegeneration(player); break;
            default:
                Printf(TEXTCOLOR_YELLOW "Upgrade %s not implemented\n", item->name.c_str());
        }
    }
    // Special Items (6000-6999)
    else if (baseId >= 6000 && baseId < 7000) {
        switch (baseId) {
            case 6000: RevealFullMap(); break;
            case 6001: HighlightSecrets(); break;
            case 6002: GrantCheckpointToken(player); break;
            case 6003: GiveBackpackToPlayer(player); break;
            case 6004: GivePowerupToPlayer(player, "PowerStrength"); break;
            case 6005: GivePowerupToPlayer(player, "PowerTimeFreezer"); break;
            default:
                Printf(TEXTCOLOR_YELLOW "Special item %s not implemented\n", item->name.c_str());
        }
    }
    // Progressive Items (7000-7999)
    else if (baseId >= 7000 && baseId < 8000) {
        switch (baseId) {
            case 7000: GiveProgressiveWeapon(player); break;
            case 7001: GiveProgressiveArmor(player); break;
            case 7002: GiveProgressiveAccess(player); break;
            case 7003: GiveProgressiveAbility(player); break;
            default:
                Printf(TEXTCOLOR_YELLOW "Progressive item %s not implemented\n", item->name.c_str());
        }
    }
    
    // Play item received sound
    S_Sound(CHAN_ITEM, "misc/secret", 1, ATTN_NONE);
}

void OnLocationChecked(int locationId, const char* locationName) {
    // Prevent duplicate location checks
    if (g_checkedLocations.find(locationId) != g_checkedLocations.end()) {
        return;
    }
    
    g_checkedLocations.insert(locationId);
    
    const LocationDef* location = GetLocationById(locationId);
    if (!location) {
        Printf(TEXTCOLOR_RED "Unknown location ID: %d\n", locationId);
        return;
    }
    
    Printf(TEXTCOLOR_CYAN "Location checked: %s\n", location->name.c_str());
    
    // Send location check to Archipelago server
    if (g_archipelagoSocket && g_archipelagoSocket->IsConnected()) {
        // Format location check message
        std::stringstream json;
        json << "[{\"cmd\":\"LocationChecks\",\"locations\":[" << locationId << "]}]";
        
        ArchipelagoMessage msg;
        msg.type = ArchipelagoMessageType::DATA;
        msg.data = json.str();
        
        if (!g_archipelagoSocket->SendMessage(msg)) {
            Printf(TEXTCOLOR_RED "Failed to send location check\n");
        }
    }
}

void OnGoalCompleted(int goalId) {
    Printf(TEXTCOLOR_GOLD "Archipelago: Goal %d completed!\n", goalId);
    
    // Handle victory condition
    if (goalId == 0) { // Main goal
        Printf(TEXTCOLOR_GOLD "Congratulations! You have completed Selaco in Archipelago!\n");
        // Trigger ending sequence
    }
}

void OnPlayerConnected(const char* playerName) {
    Printf(TEXTCOLOR_GREEN "%s has connected to the multiworld\n", playerName);
}

void OnPlayerDisconnected(const char* playerName) {
    Printf(TEXTCOLOR_ORANGE "%s has disconnected from the multiworld\n", playerName);
}

} // namespace ArchipelagoEvents

// Helper functions for granting items

static void GiveWeaponToPlayer(AActor* player, const char* weaponClass) {
    if (!player) return;
    
    auto weaponType = PClass::FindActor(weaponClass);
    if (!weaponType) {
        Printf(TEXTCOLOR_RED "Unknown weapon class: %s\n", weaponClass);
        return;
    }
    
    auto weapon = player->GiveInventoryType(weaponType);
    if (weapon) {
        Printf("Gave weapon: %s\n", weaponClass);
    }
}

static void GiveAmmoToPlayer(AActor* player, const char* ammoClass, int amount) {
    if (!player) return;
    
    auto ammoType = PClass::FindActor(ammoClass);
    if (!ammoType) {
        Printf(TEXTCOLOR_RED "Unknown ammo class: %s\n", ammoClass);
        return;
    }
    
    player->GiveAmmo(ammoType, amount);
    Printf("Gave %d %s\n", amount, ammoClass);
}

static void GiveHealthToPlayer(AActor* player, int amount) {
    if (!player) return;
    
    player->health = MIN(player->health + amount, player->GetMaxHealth(true));
    player->player->health = player->health;
    Printf("Gave %d health\n", amount);
}

static void GiveArmorToPlayer(AActor* player, int amount) {
    if (!player) return;
    
    auto armorType = PClass::FindActor("BasicArmor");
    if (!armorType) return;
    
    auto armor = player->FindInventory(armorType);
    if (!armor) {
        armor = player->GiveInventoryType(armorType);
    }
    
    if (armor) {
        armor->Amount = MIN(armor->Amount + amount, armor->MaxAmount);
        Printf("Gave %d armor\n", amount);
    }
}

static void GiveMegaHealthToPlayer(AActor* player) {
    if (!player) return;
    
    player->health = MIN(player->health + 100, 200);
    player->player->health = player->health;
    Printf("Gave Mega Health\n");
}

static void GiveInvulnerabilityToPlayer(AActor* player, int duration) {
    if (!player) return;
    
    auto invulnType = PClass::FindActor("PowerInvulnerable");
    if (!invulnType) return;
    
    auto powerup = player->GiveInventoryType(invulnType);
    if (powerup) {
        powerup->EffectTics = duration * TICRATE;
        Printf("Gave invulnerability for %d seconds\n", duration);
    }
}

static void GiveKeyToPlayer(AActor* player, const char* keyClass) {
    if (!player) return;
    
    auto keyType = PClass::FindActor(keyClass);
    if (!keyType) {
        Printf(TEXTCOLOR_RED "Unknown key class: %s\n", keyClass);
        return;
    }
    
    auto key = player->GiveInventoryType(keyType);
    if (key) {
        Printf("Gave key: %s\n", keyClass);
    }
}

static void GivePowerupToPlayer(AActor* player, const char* powerupClass) {
    if (!player) return;
    
    auto powerupType = PClass::FindActor(powerupClass);
    if (!powerupType) {
        Printf(TEXTCOLOR_RED "Unknown powerup class: %s\n", powerupClass);
        return;
    }
    
    auto powerup = player->GiveInventoryType(powerupType);
    if (powerup) {
        Printf("Gave powerup: %s\n", powerupClass);
    }
}

static void GiveBackpackToPlayer(AActor* player) {
    if (!player) return;
    
    auto backpackType = PClass::FindActor("Backpack");
    if (!backpackType) return;
    
    player->GiveInventoryType(backpackType);
    Printf("Gave backpack - ammo capacity increased\n");
}

// Game modification functions

static void ApplyDamageBoost(AActor* player, float multiplier) {
    if (!player) return;
    
    player->DamageMultiply = multiplier;
    Printf("Damage multiplier set to %.2fx\n", multiplier);
}

static void ApplySpeedBoost(AActor* player, float multiplier) {
    if (!player) return;
    
    player->Speed = player->GetDefault()->Speed * multiplier;
    Printf("Speed multiplier set to %.2fx\n", multiplier);
}

static void EnableJumpBoots(AActor* player) {
    if (!player || !player->player) return;
    
    player->player->jumpTics = -1; // Enable double jump
    Printf("Jump boots enabled\n");
}

static void EnableRadarScanner(AActor* player) {
    if (!player) return;
    
    // Set scanner flag
    player->player->cheats |= CF_ALLMAP;
    Printf("Radar scanner enabled\n");
}

static void EnableAmmoConverter(AActor* player) {
    if (!player) return;
    
    // This would need custom implementation
    Printf("Ammo converter enabled (not fully implemented)\n");
}

static void EnableHealthRegeneration(AActor* player) {
    if (!player) return;
    
    // This would need custom implementation
    Printf("Health regeneration enabled (not fully implemented)\n");
}

static void RevealFullMap() {
    // Reveal all map lines
    level.flags2 |= LEVEL2_ALLMAP;
    Printf("Map fully revealed\n");
}

static void HighlightSecrets() {
    // This would need custom implementation
    Printf("Secret areas highlighted (not fully implemented)\n");
}

static void GrantCheckpointToken(AActor* player) {
    if (!player) return;
    
    // This would need custom implementation
    Printf("Checkpoint token granted (not fully implemented)\n");
}

// Progressive item tracking
static int g_progressiveWeaponLevel = 0;
static int g_progressiveArmorLevel = 0;
static int g_progressiveAccessLevel = 0;
static int g_progressiveAbilityLevel = 0;

static void GiveProgressiveWeapon(AActor* player) {
    const char* weapons[] = {
        "Pistol", "Shotgun", "AssaultRifle", "GrenadeLauncher",
        "RocketLauncher", "PlasmaRifle", "RailGun", "BFG9000"
    };
    
    if (g_progressiveWeaponLevel < 8) {
        GiveWeaponToPlayer(player, weapons[g_progressiveWeaponLevel]);
        g_progressiveWeaponLevel++;
    }
}

static void GiveProgressiveArmor(AActor* player) {
    int armorAmounts[] = { 25, 50, 75, 100, 150, 200 };
    
    if (g_progressiveArmorLevel < 6) {
        GiveArmorToPlayer(player, armorAmounts[g_progressiveArmorLevel]);
        g_progressiveArmorLevel++;
    }
}

static void GiveProgressiveAccess(AActor* player) {
    const char* keys[] = {
        "RedCard", "BlueCard", "YellowCard",
        "SecurityAlpha", "SecurityBeta", "SecurityOmega"
    };
    
    if (g_progressiveAccessLevel < 6) {
        GiveKeyToPlayer(player, keys[g_progressiveAccessLevel]);
        g_progressiveAccessLevel++;
    }
}

static void GiveProgressiveAbility(AActor* player) {
    switch (g_progressiveAbilityLevel) {
        case 0: ApplySpeedBoost(player, 1.1f); break;
        case 1: EnableJumpBoots(player); break;
        case 2: ApplyDamageBoost(player, 1.15f); break;
        case 3: EnableRadarScanner(player); break;
        case 4: EnableHealthRegeneration(player); break;
        default: break;
    }
    
    if (g_progressiveAbilityLevel < 5) {
        g_progressiveAbilityLevel++;
    }
}

// Console commands for testing

CCMD(archipelago_listitems) {
    Printf("=== Archipelago Items for Selaco ===\n");
    Printf("ID         | Name                      | Description\n");
    Printf("-----------|---------------------------|----------------------------\n");
    
    for (const auto& item : SELACO_ITEMS) {
        Printf("%10lld | %-25s | %s\n", 
               static_cast<long long>(item.id),
               item.name.c_str(), 
               item.description.c_str());
    }
    
    Printf("\nTotal items: %zu\n", SELACO_ITEMS.size());
}

CCMD(archipelago_listlocations) {
    Printf("=== Archipelago Locations for Selaco ===\n");
    Printf("ID         | Name                      | Description\n");
    Printf("-----------|---------------------------|----------------------------\n");
    
    for (const auto& loc : SELACO_LOCATIONS) {
        Printf("%10lld | %-25s | %s\n", 
               static_cast<long long>(loc.id),
               loc.name.c_str(), 
               loc.description.c_str());
    }
    
    Printf("\nTotal locations: %zu\n", SELACO_LOCATIONS.size());
}

CCMD(archipelago_testitem) {
    if (argv.argc() < 2) {
        Printf("Usage: archipelago_testitem <item_id>\n");
        Printf("Example: archipelago_testitem %lld (for Pistol)\n", 
               static_cast<long long>(SELACO_BASE_ID + 1000));
        return;
    }
    
    int64_t itemId = static_cast<int64_t>(strtoull(argv[1], nullptr, 0));
    const ItemDef* item = GetItemById(itemId);
    
    if (item) {
        ArchipelagoEvents::OnItemReceived(itemId, item->name.c_str());
    } else {
        Printf(TEXTCOLOR_RED "Invalid item ID: %lld\n", static_cast<long long>(itemId));
    }
}

CCMD(archipelago_testlocation) {
    if (argv.argc() < 2) {
        Printf("Usage: archipelago_testlocation <location_id>\n");
        Printf("Example: archipelago_testlocation %lld (for Complete Chapter 1)\n",
               static_cast<long long>(SELACO_BASE_ID + 10000));
        return;
    }
    
    int64_t locationId = static_cast<int64_t>(strtoull(argv[1], nullptr, 0));
    const LocationDef* location = GetLocationById(locationId);
    
    if (location) {
        ArchipelagoEvents::OnLocationChecked(locationId, location->name.c_str());
    } else {
        Printf(TEXTCOLOR_RED "Invalid location ID: %lld\n", static_cast<long long>(locationId));
    }
}

CCMD(archipelago_clearitems) {
    g_receivedItems.clear();
    g_checkedLocations.clear();
    g_progressiveWeaponLevel = 0;
    g_progressiveArmorLevel = 0;
    g_progressiveAccessLevel = 0;
    g_progressiveAbilityLevel = 0;
    Printf("Cleared all received items and checked locations\n");
}

CCMD(archipelago_status) {
    Printf("=== Archipelago Status ===\n");
    Printf("Received items: %zu\n", g_receivedItems.size());
    Printf("Checked locations: %zu\n", g_checkedLocations.size());
    Printf("Progressive levels:\n");
    Printf("  Weapon: %d/8\n", g_progressiveWeaponLevel);
    Printf("  Armor: %d/6\n", g_progressiveArmorLevel);
    Printf("  Access: %d/6\n", g_progressiveAccessLevel);
    Printf("  Ability: %d/5\n", g_progressiveAbilityLevel);
}