// Archipelago Menu Implementation for Selaco
#include "menu/menu.h"
#include "c_dispatch.h"
#include "c_cvars.h"
#include "v_text.h"
#include "v_video.h"
#include "gi.h"
#include "g_game.h"
#include "s_sound.h"

// External CVars from archipelago_commands.cpp
EXTERN_CVAR(String, archipelago_host)
EXTERN_CVAR(Int, archipelago_port)
EXTERN_CVAR(String, archipelago_slot)
EXTERN_CVAR(String, archipelago_password)

// Forward declarations
void Archipelago_Connect();
void Archipelago_Disconnect();
bool Archipelago_IsConnected();

//=============================================================================
//
// Archipelago Menu Console Command
//
//=============================================================================

CCMD(menu_archipelago)
{
    // Play menu activation sound
    S_Sound(CHAN_VOICE, CHANF_UI, "menu/activate", snd_menuvolume, ATTN_NONE);
    
    // Start the control panel if not already active
    M_StartControlPanel(true);
    
    // Set the archipelago menu
    M_SetMenu("ArchipelagoMenu", -1);
}

//=============================================================================
//
// Connection Handler Functions
//
//=============================================================================

void ArchipelagoMenu_Connect()
{
    // Validate input
    if (strlen(archipelago_slot) == 0)
    {
        Printf(TEXTCOLOR_RED "Error: Slot name is required!\n");
        S_Sound(CHAN_VOICE, CHANF_UI, "menu/invalid", snd_menuvolume, ATTN_NONE);
        return;
    }
    
    // Build and execute the connection command
    FString connectCmd;
    connectCmd.Format("archipelago_connect \"%s\" \"%s:%d\" \"%s\"",
        (const char*)archipelago_slot,
        (const char*)archipelago_host,
        (int)archipelago_port,
        (const char*)archipelago_password);
    
    // Close the menu
    M_ClearMenus();
    
    // Execute the connection command
    C_DoCommand(connectCmd);
}

void ArchipelagoMenu_Disconnect()
{
    C_DoCommand("archipelago_disconnect");
    
    // Play disconnect sound
    S_Sound(CHAN_VOICE, CHANF_UI, "menu/dismiss", snd_menuvolume, ATTN_NONE);
    
    // Return to archipelago menu
    M_SetMenu("ArchipelagoMenu", -1);
}

void ArchipelagoMenu_ShowStatus()
{
    C_DoCommand("archipelago_status");
}

//=============================================================================
//
// Menu Action Functions (called from ZScript)
//
//=============================================================================

DEFINE_ACTION_FUNCTION(_ArchipelagoMenu, Connect)
{
    PARAM_PROLOGUE;
    ArchipelagoMenu_Connect();
    return 0;
}

DEFINE_ACTION_FUNCTION(_ArchipelagoMenu, Disconnect)
{
    PARAM_PROLOGUE;
    ArchipelagoMenu_Disconnect();
    return 0;
}

DEFINE_ACTION_FUNCTION(_ArchipelagoMenu, ShowStatus)
{
    PARAM_PROLOGUE;
    ArchipelagoMenu_ShowStatus();
    return 0;
}

DEFINE_ACTION_FUNCTION(_ArchipelagoMenu, IsConnected)
{
    PARAM_PROLOGUE;
    // Check if we're connected by trying to get status
    // This is a simplified check - you may want to expose the actual connection state
    ACTION_RETURN_BOOL(false); // Placeholder - implement actual connection check
}

//=============================================================================
//
// Initialize Archipelago Menu System
//
//=============================================================================

void InitArchipelagoMenu()
{
    // Any initialization code for the archipelago menu system
    Printf("Archipelago menu system initialized\n");
}