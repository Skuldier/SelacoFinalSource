#pragma once

// Main header for Archipelago integration into Selaco
// Include this in your main game loop and initialization code

// Initialize the Archipelago system
// Call this during game startup (e.g., in D_DoomMain or similar)
void Archipelago_Init();

// Shutdown the Archipelago system
// Call this during game shutdown
void Archipelago_Shutdown();

// Process incoming Archipelago messages
// Call this every frame or tick in your main game loop
void Archipelago_ProcessMessages();

// Optional: Register Archipelago-specific game events
// These would be implemented based on your game's specific needs
namespace ArchipelagoEvents {
    // Example event handlers - implement as needed
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