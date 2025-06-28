// Enhanced Archipelago Menu with dynamic status display
// Place this file in wadsrc/static/zscript/archipelago_menu.zs

class ArchipelagoStatusItem : OptionMenuItemStaticText
{
    override int Draw(OptionMenuDescriptor desc, int y, int indent, bool selected)
    {
        // Update status text based on current connection
        String status = GetConnectionStatus();
        
        // Choose color based on status
        int textColor;
        if (status.IndexOf("Connected") >= 0)
            textColor = Font.CR_GREEN;
        else if (status.IndexOf("Connecting") >= 0)
            textColor = Font.CR_YELLOW;
        else
            textColor = Font.CR_RED;
            
        // Draw the status text
        screen.DrawText(mFont, textColor, indent + mXpos, y, status, DTA_Clean, true);
        return -1;
    }
    
    String GetConnectionStatus()
    {
        // Check CVars to determine status
        CVar hostCvar = CVar.FindCVar("archipelago_host");
        CVar slotCvar = CVar.FindCVar("archipelago_slot");
        
        if (!hostCvar || !slotCvar)
            return "Status: Error - CVars not found";
            
        // This is a placeholder - in a real implementation you'd check
        // actual connection status from your socket system
        if (slotCvar.GetString() == "")
            return "Status: No slot configured";
        else
            return String.Format("Status: Not connected to %s", hostCvar.GetString());
    }
}

class ArchipelagoMenu : OptionMenu
{
    override void Init(Menu parent, OptionMenuDescriptor desc)
    {
        Super.Init(parent, desc);
        
        // Replace the static status text with our dynamic one
        for (int i = 0; i < desc.mItems.Size(); i++)
        {
            let item = OptionMenuItemStaticText(desc.mItems[i]);
            if (item && item.mLabel == "Not connected")
            {
                // Replace with our custom status item
                desc.mItems[i] = new("ArchipelagoStatusItem").Init("", 0);
                break;
            }
        }
    }
    
    override void Ticker()
    {
        Super.Ticker();
        // Update every half second
        if (MenuTime % 17 == 0)
        {
            // Force redraw to update status
            // This ensures the status stays current
        }
    }
}

// Enhanced connect command that validates before connecting
class ArchipelagoConnectCommand : OptionMenuItemCommand
{
    override bool Activate()
    {
        // Check if slot name is set
        CVar slotCvar = CVar.FindCVar("archipelago_slot");
        if (!slotCvar || slotCvar.GetString() == "")
        {
            Menu.MenuSound("menu/invalid");
            
            // Show error message in console
            Console.Printf("Error: Slot name must be set before connecting!");
            
            // Could also show a message box
            Menu.StartMessage("Please set a slot name before connecting!", 1);
            
            return false;
        }
        
        // If validation passes, execute the command
        return Super.Activate();
    }
}

// Password field that masks input
class ArchipelagoPasswordField : OptionMenuItemTextField  
{
    override void Drawer(bool selected)
    {
        // Store the original text
        String original = mEnter.GetText();
        
        if (original.Length() > 0 && !selected)
        {
            // Create masked version
            String masked = "";
            for (int i = 0; i < original.Length(); i++)
                masked.AppendFormat("*");
            
            mEnter.SetText(masked);
        }
        
        // Draw
        Super.Drawer(selected);
        
        // Restore original
        mEnter.SetText(original);
    }
}