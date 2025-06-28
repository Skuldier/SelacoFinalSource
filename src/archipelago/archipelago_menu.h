#ifndef __ARCHIPELAGO_MENU_H__
#define __ARCHIPELAGO_MENU_H__

#include "c_cvars.h"

// External CVars for Archipelago settings
EXTERN_CVAR(String, archipelago_host)
EXTERN_CVAR(Int, archipelago_port)
EXTERN_CVAR(String, archipelago_slot)
EXTERN_CVAR(String, archipelago_password)
EXTERN_CVAR(Bool, archipelago_autoconnect)
EXTERN_CVAR(Bool, archipelago_debug)

// Menu initialization
void InitArchipelagoMenu();

// Required includes for menu.cpp
void M_ClearMenus();
void M_StartControlPanel(bool makeSound);
void M_SetMenu(const char* menu, int param = -1);

#endif // __ARCHIPELAGO_MENU_H__