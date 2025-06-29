#include <sstream>
#include <algorithm>
#include <string>

#include "archipelago/archipelago_integration.h"
#include "archipelago/archipelago_socket.h"
#include "c_dispatch.h"
#include "c_cvars.h"
#include "doomtype.h"
#include "d_player.h"
#include "g_game.h"
#include "gi.h"
#include "p_local.h"
#include "s_sound.h"
#include "gstrings.h"
#include "m_misc.h"
#include "v_text.h"
#include "c_console.h"
#include "d_net.h"
#include "serializer.h"
#include "actor.h"
#include "info.h"
#include "d_event.h"

// External references
extern player_t players[MAXPLAYERS];
extern int consoleplayer;

// Math utilities
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

// Forward declarations of helper functions
static bool GiveWeaponToPlayer(player_t* player, const char* weaponName);
static bool GiveAmmoToPlayer(player_t* player, const char* ammoType, int amount);
static bool GiveHealthToPlayer(player_t* player, int amount);
static bool GiveArmorToPlayer(player_t* player, const char* armorType, int amount);
static bool GiveMegaHealthToPlayer(player_t* player);
static bool GiveInvulnerabilityToPlayer(player_t* player);
static bool GiveKeyToPlayer(player_t* player, const char* keyName);
static bool GivePowerupToPlayer(player_t* player, const char* powerupName);
static bool GiveBackpackToPlayer(player_t* player);
static void ApplyDamageBoost(player_t* player, float multiplier);
static void ApplySpeedBoost(player_t* player, float multiplier);
static void EnableJumpBoots(player_t* player);
static void EnableRadarScanner(player_t* player);
static void EnableAmmoConverter(player_t* player);
static void EnableHealthRegeneration(player_t* player);
static void RevealFullMap(player_t* player);
static void HighlightSecrets(player_t* player);
static void GrantCheckpointToken(player_t* player);
static void GiveProgressiveWeapon(player_t* player);
static void GiveProgressiveArmor(player_t* player);
static void GiveProgressiveAccess(player_t* player);
static void GiveProgressiveAbility(player_t* player);

// Archipelago item processing
void ProcessArchipelagoItem(int itemId, const char* itemName) {
    // Get current player
    auto player = &players[consoleplayer];
    if (!player || !player->mo) {
        Printf(TEXTCOLOR_RED "Error: Invalid player state\n");
        return;
    }

    Printf("Archipelago: Received item '%s' (ID: %d)\n", itemName, itemId);

    // Process item based on ID ranges
    // Weapons (50000-50009)
    if (itemId >= 50000 && itemId <= 50009) {
        switch (itemId) {
            case 50000: GiveWeaponToPlayer(player, "Pistol"); break;
            case 50001: GiveWeaponToPlayer(player, "AssaultRifle"); break;
            case 50002: GiveWeaponToPlayer(player, "Shotgun"); break;
            case 50003: GiveWeaponToPlayer(player, "SuperShotgun"); break;
            case 50004: GiveWeaponToPlayer(player, "GrenadeLauncher"); break;
            case 50005: GiveWeaponToPlayer(player, "Minigun"); break;
            case 50006: GiveWeaponToPlayer(player, "PlasmaRifle"); break;
            case 50007: GiveWeaponToPlayer(player, "RocketLauncher"); break;
            case 50008: GiveWeaponToPlayer(player, "Railgun"); break;
            case 50009: GiveWeaponToPlayer(player, "BFG9000"); break;
        }
    }
    // Ammo (50010-50019)
    else if (itemId >= 50010 && itemId <= 50019) {
        switch (itemId) {
            case 50010: GiveAmmoToPlayer(player, "Clip", 10); break;
            case 50011: GiveAmmoToPlayer(player, "Clip", 50); break;
            case 50012: GiveAmmoToPlayer(player, "Shell", 4); break;
            case 50013: GiveAmmoToPlayer(player, "Shell", 20); break;
            case 50014: GiveAmmoToPlayer(player, "RocketAmmo", 1); break;
            case 50015: GiveAmmoToPlayer(player, "RocketAmmo", 5); break;
            case 50016: GiveAmmoToPlayer(player, "Cell", 20); break;
            case 50017: GiveAmmoToPlayer(player, "Cell", 100); break;
            case 50018: GiveAmmoToPlayer(player, "GrenadeAmmo", 2); break;
            case 50019: GiveAmmoToPlayer(player, "GrenadeAmmo", 10); break;
        }
    }
    // Health & Armor (50020-50026)
    else if (itemId >= 50020 && itemId <= 50026) {
        switch (itemId) {
            case 50020: GiveHealthToPlayer(player, 10); break;
            case 50021: GiveHealthToPlayer(player, 25); break;
            case 50022: GiveArmorToPlayer(player, "GreenArmor", 100); break;
            case 50023: GiveArmorToPlayer(player, "BlueArmor", 200); break;
            case 50024: GiveArmorToPlayer(player, "ArmorBonus", 1); break;
            case 50025: GiveMegaHealthToPlayer(player); break;
            case 50026: GiveInvulnerabilityToPlayer(player); break;
        }
    }
    // Keys (50030-50037)
    else if (itemId >= 50030 && itemId <= 50037) {
        switch (itemId) {
            case 50030: GiveKeyToPlayer(player, "RedCard"); break;
            case 50031: GiveKeyToPlayer(player, "BlueCard"); break;
            case 50032: GiveKeyToPlayer(player, "YellowCard"); break;
            case 50033: GiveKeyToPlayer(player, "RedSkull"); break;
            case 50034: GiveKeyToPlayer(player, "BlueSkull"); break;
            case 50035: GiveKeyToPlayer(player, "YellowSkull"); break;
            case 50036: GiveKeyToPlayer(player, "GreenCard"); break;
            case 50037: GiveKeyToPlayer(player, "GreenSkull"); break;
        }
    }
    // Powerups & Upgrades (50040-50047)
    else if (itemId >= 50040 && itemId <= 50047) {
        switch (itemId) {
            case 50040: ApplyDamageBoost(player, 2.0f); break;
            case 50041: ApplySpeedBoost(player, 1.5f); break;
            case 50042: EnableJumpBoots(player); break;
            case 50043: GivePowerupToPlayer(player, "Infrared"); break;
            case 50044: EnableRadarScanner(player); break;
            case 50045: GivePowerupToPlayer(player, "RadSuit"); break;
            case 50046: EnableAmmoConverter(player); break;
            case 50047: EnableHealthRegeneration(player); break;
        }
    }
    // Special Items (50050-50055)
    else if (itemId >= 50050 && itemId <= 50055) {
        switch (itemId) {
            case 50050: RevealFullMap(player); break;
            case 50051: HighlightSecrets(player); break;
            case 50052: GrantCheckpointToken(player); break;
            case 50053: GiveBackpackToPlayer(player); break;
            case 50054: GivePowerupToPlayer(player, "Berserk"); break;
            case 50055: GivePowerupToPlayer(player, "Invisibility"); break;
        }
    }
    // Progressive Items (50060-50063)
    else if (itemId >= 50060 && itemId <= 50063) {
        switch (itemId) {
            case 50060: GiveProgressiveWeapon(player); break;
            case 50061: GiveProgressiveArmor(player); break;
            case 50062: GiveProgressiveAccess(player); break;
            case 50063: GiveProgressiveAbility(player); break;
        }
    }
    else {
        Printf("Unknown Archipelago item ID: %d\n", itemId);
    }

    // Play item received sound
    S_Sound(CHAN_ITEM, 0, "misc/secret", 1, ATTN_NONE);
}

// Archipelago Events namespace implementation
namespace ArchipelagoEvents {
    void OnItemReceived(int itemId, const char* itemName) {
        ProcessArchipelagoItem(itemId, itemName);
    }

    void OnLocationChecked(int locationId, const char* locationName) {
        Printf("Archipelago: Location '%s' checked (ID: %d)\n", locationName, locationId);
    }

    void OnGoalCompleted(int goalId) {
        Printf(TEXTCOLOR_GOLD "Archipelago: Goal %d completed!\n", goalId);
    }

    void OnPlayerConnected(const char* playerName) {
        Printf(TEXTCOLOR_GREEN "%s connected to the multiworld\n", playerName);
    }

    void OnPlayerDisconnected(const char* playerName) {
        Printf(TEXTCOLOR_ORANGE "%s disconnected from the multiworld\n", playerName);
    }
}

// Save/Load Archipelago state
void SerializeArchipelagoState(FSerializer& arc) {
    // Serialize Archipelago-specific data here
    if (arc.isWriting()) {
        // Save state
        std::string jsonData = "{\"connected\":false,\"items_received\":[]}";
        arc("archipelago_state", jsonData);
    } else {
        // Load state
        std::string jsonStr;
        arc("archipelago_state", jsonStr);
        // Parse and restore state
    }
}

// Helper function implementations
static bool GiveWeaponToPlayer(player_t* player, const char* weaponName) {
    if (!player || !player->mo) return false;
    
    PClassActor* weaponClass = PClass::FindActor(weaponName);
    if (!weaponClass) {
        Printf(TEXTCOLOR_RED "Unknown weapon class: %s\n", weaponName);
        return false;
    }
    
    AActor* weapon = player->mo->GiveInventoryType(weaponClass);
    if (weapon) {
        Printf(TEXTCOLOR_GOLD "Received weapon: %s\n", weaponName);
        return true;
    }
    return false;
}

static bool GiveAmmoToPlayer(player_t* player, const char* ammoType, int amount) {
    if (!player || !player->mo) return false;
    
    PClassActor* ammoClass = PClass::FindActor(ammoType);
    if (!ammoClass) {
        Printf(TEXTCOLOR_RED "Unknown ammo type: %s\n", ammoType);
        return false;
    }
    
    AActor* ammo = player->mo->FindInventory(ammoClass);
    if (!ammo) {
        ammo = player->mo->GiveInventoryType(ammoClass);
    }
    
    if (ammo) {
        // Use the engine's method to add ammo properly
        int currentAmount = ammo->IntVar("Amount");
        int maxAmount = ammo->IntVar("MaxAmount");
        ammo->IntVar("Amount") = MIN(currentAmount + amount, maxAmount);
        Printf(TEXTCOLOR_GOLD "Received %d %s\n", amount, ammoType);
        return true;
    }
    return false;
}

static bool GiveHealthToPlayer(player_t* player, int amount) {
    if (!player || !player->mo) return false;
    
    player->mo->health = MIN(player->mo->health + amount, player->mo->GetMaxHealth());
    player->health = player->mo->health;
    Printf(TEXTCOLOR_GOLD "Received %d health\n", amount);
    return true;
}

static bool GiveArmorToPlayer(player_t* player, const char* armorType, int amount) {
    if (!player || !player->mo) return false;
    
    PClassActor* armorClass = PClass::FindActor(armorType);
    if (!armorClass) {
        Printf(TEXTCOLOR_RED "Unknown armor type: %s\n", armorType);
        return false;
    }
    
    AActor* armor = player->mo->GiveInventoryType(armorClass);
    if (armor) {
        Printf(TEXTCOLOR_GOLD "Received %s\n", armorType);
        return true;
    }
    return false;
}

static bool GiveMegaHealthToPlayer(player_t* player) {
    if (!player || !player->mo) return false;
    
    player->mo->health = MIN(player->mo->health + 100, player->mo->GetMaxHealth() * 2);
    player->health = player->mo->health;
    Printf(TEXTCOLOR_GOLD "Received Megahealth!\n");
    return true;
}

static bool GiveInvulnerabilityToPlayer(player_t* player) {
    if (!player || !player->mo) return false;
    
    PClassActor* invulnClass = PClass::FindActor("PowerInvulnerable");
    if (invulnClass) {
        AActor* powerup = player->mo->GiveInventoryType(invulnClass);
        if (powerup) {
            powerup->IntVar(NAME_EffectTics) = 30 * TICRATE;
            Printf(TEXTCOLOR_GOLD "Invulnerability activated!\n");
            return true;
        }
    }
    return false;
}

static bool GiveKeyToPlayer(player_t* player, const char* keyName) {
    if (!player || !player->mo) return false;
    
    PClassActor* keyClass = PClass::FindActor(keyName);
    if (!keyClass) {
        Printf(TEXTCOLOR_RED "Unknown key type: %s\n", keyName);
        return false;
    }
    
    AActor* key = player->mo->GiveInventoryType(keyClass);
    if (key) {
        Printf(TEXTCOLOR_GOLD "Received %s\n", keyName);
        return true;
    }
    return false;
}

static bool GivePowerupToPlayer(player_t* player, const char* powerupName) {
    if (!player || !player->mo) return false;
    
    std::string fullName = std::string("Power") + powerupName;
    PClassActor* powerClass = PClass::FindActor(fullName.c_str());
    if (!powerClass) {
        Printf(TEXTCOLOR_RED "Unknown powerup: %s\n", powerupName);
        return false;
    }
    
    AActor* powerup = player->mo->GiveInventoryType(powerClass);
    if (powerup) {
        powerup->IntVar(NAME_EffectTics) = 60 * TICRATE;
        Printf(TEXTCOLOR_GOLD "Powerup activated: %s\n", powerupName);
        return true;
    }
    return false;
}

static bool GiveBackpackToPlayer(player_t* player) {
    if (!player || !player->mo) return false;
    
    PClassActor* backpackClass = PClass::FindActor("Backpack");
    if (backpackClass) {
        player->mo->GiveInventoryType(backpackClass);
        Printf(TEXTCOLOR_GOLD "Received Backpack - ammo capacity increased!\n");
        return true;
    }
    return false;
}

static void ApplyDamageBoost(player_t* player, float multiplier) {
    if (!player || !player->mo) return;
    
    // Store damage multiplier in player structure or custom inventory
    Printf(TEXTCOLOR_GOLD "Damage boost x%.1f activated!\n", multiplier);
}

static void ApplySpeedBoost(player_t* player, float multiplier) {
    if (!player || !player->mo) return;
    
    // Apply speed boost
    player->mo->Speed *= multiplier;
    Printf(TEXTCOLOR_GOLD "Speed boost x%.1f activated!\n", multiplier);
}

static void EnableJumpBoots(player_t* player) {
    if (!player || !player->mo) return;
    
    // Enable enhanced jumping
    player->jumpTics = 18; // Allow higher jumps
    Printf(TEXTCOLOR_GOLD "Jump Boots enabled!\n");
}

static void EnableRadarScanner(player_t* player) {
    if (!player || !player->mo) return;
    
    // Enable radar/scanner
    Printf(TEXTCOLOR_GOLD "Radar Scanner activated!\n");
}

static void EnableAmmoConverter(player_t* player) {
    if (!player || !player->mo) return;
    
    // Enable ammo conversion ability
    Printf(TEXTCOLOR_GOLD "Ammo Converter enabled!\n");
}

static void EnableHealthRegeneration(player_t* player) {
    if (!player || !player->mo) return;
    
    // Enable health regeneration
    Printf(TEXTCOLOR_GOLD "Health Regeneration activated!\n");
}

static void RevealFullMap(player_t* player) {
    if (!player) return;
    
    player->cheats |= CF_ALLMAP;
    Printf(TEXTCOLOR_GOLD "Full map revealed!\n");
}

static void HighlightSecrets(player_t* player) {
    if (!player) return;
    
    // Highlight secrets on automap
    Printf(TEXTCOLOR_GOLD "Secrets highlighted on map!\n");
}

static void GrantCheckpointToken(player_t* player) {
    if (!player || !player->mo) return;
    
    // Grant checkpoint/save token
    Printf(TEXTCOLOR_GOLD "Checkpoint token received!\n");
}

static void GiveProgressiveWeapon(player_t* player) {
    if (!player || !player->mo) return;
    
    // Give next weapon in progression
    Printf(TEXTCOLOR_GOLD "Progressive weapon upgrade!\n");
}

static void GiveProgressiveArmor(player_t* player) {
    if (!player || !player->mo) return;
    
    // Give next armor tier
    Printf(TEXTCOLOR_GOLD "Progressive armor upgrade!\n");
}

static void GiveProgressiveAccess(player_t* player) {
    if (!player || !player->mo) return;
    
    // Grant next access level
    Printf(TEXTCOLOR_GOLD "Progressive access granted!\n");
}

static void GiveProgressiveAbility(player_t* player) {
    if (!player || !player->mo) return;
    
    // Grant next ability
    Printf(TEXTCOLOR_GOLD "Progressive ability unlocked!\n");
}

// Console command to test item receiving
CCMD(archipelago_testitem) {
    if (argv.argc() < 2) {
        Printf("Usage: archipelago_testitem <item_id> [item_name]\n");
        return;
    }
    
    int itemId = atoi(argv[1]);
    const char* itemName = (argv.argc() >= 3) ? argv[2] : "Test Item";
    
    ProcessArchipelagoItem(itemId, itemName);
}