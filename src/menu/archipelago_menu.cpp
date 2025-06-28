// Archipelago Menu Implementation for Selaco
#include "common/menu/menu.h"
#include "c_dispatch.h"
#include "c_cvars.h"
#include "v_text.h"
#include "gi.h"
#include "g_game.h"
#include "s_sound.h"
#include "doomstat.h"  // For sound channel definitions

// External CVars from archipelago_commands.cpp
EXTERN_CVAR(String, archipelago_host)
EXTERN_CVAR(Int, archipelago_port)
EXTERN_CVAR(String, archipelago_slot)
EXTERN_CVAR(String, archipelago_password)
EXTERN_CVAR(Float, snd_menuvolume)

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
// Menu Commands for MENUDEF
//
//=============================================================================

CCMD(archipelago_connect_menu)
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
    C_DoCommand(connectCmd.GetChars());
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