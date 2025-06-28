#pragma once

// Main header for Archipelago integration into Selaco
// Include this in your main game loop and initialization code

// Forward declarations for d_main.cpp
extern void Archipelago_Init();
extern void Archipelago_Shutdown();
extern void Archipelago_ProcessMessages();

// Optional: Register Archipelago-specific game events
// These would be implemented based on your game's specific needs
namespace ArchipelagoEvents {
    // Event handlers - implement in archipelago_selaco.cpp
    void OnItemReceived(int itemId, const char* itemName);
    void OnLocationChecked(int locationId, const char* locationName);
    void OnGoalCompleted(int goalId);
    void OnPlayerConnected(const char* playerName);
    void OnPlayerDisconnected(const char* playerName);
}

// Integration points for Selaco
// Add these calls to appropriate places in your codebase:
//
// 1. In main initialization (D_DoomMain or equivalent):
//    Archipelago_Init();
//
// 2. In main game loop (D_ProcessEvents or equivalent):
//    Archipelago_ProcessMessages();
//
// 3. In shutdown code:
//    Archipelago_Shutdown();
//
// 4. Console commands are automatically registered
//
// 5. To check a location in your game code:
//    ArchipelagoEvents::OnLocationChecked(locationId, locationName);
//
// 6. Items will be automatically received and processed
//    through the message processing system