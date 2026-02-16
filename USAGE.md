# CS2 Skin Changer Usage Guide

## ⚠️ CRITICAL WARNING

**Before you proceed:**
- This tool will likely result in a VAC ban
- VAC bans are permanent and affect all VAC-secured games
- Only use in offline/local environments for educational purposes
- The developers assume NO responsibility for any consequences

## Prerequisites

1. Counter-Strike 2 installed
2. CS2SkinChanger.exe built and ready
3. Configuration files (`config/offsets.json` and `config/skins.json`) present

## Step-by-Step Usage

### 1. Prepare CS2

1. Launch Counter-Strike 2
2. Either:
   - Start a practice game with bots
   - Load into any offline mode
   - Load a local server
3. **Do NOT use in online matchmaking**

### 2. Launch Skin Changer

1. Navigate to the build directory:
   ```
   build/bin/Release/
   ```

2. Run `CS2SkinChanger.exe`
   - You may need to run as Administrator
   - If Windows Defender blocks it, you may need to add an exception

3. You should see the main window with a warning message

### 3. Connect to CS2

1. Click the **"Refresh Process"** button
2. Wait for the status to show: "Success: Connected to CS2 process"
3. If it fails:
   - Ensure CS2 is running
   - Make sure you're in a game (not just in menu)
   - Try running as Administrator
   - Check if CS2 process is named `cs2.exe`

### 4. Select Weapon and Skin

1. **Choose Category**: Select from dropdown (Rifles, Pistols, Knives, SMGs)
2. **Choose Weapon**: Select specific weapon from the list
3. **Choose Skin**: Pick your desired skin from available options

Example:
- Category: Rifles
- Weapon: AK-47
- Skin: Vulcan

### 5. Apply the Skin

1. Click **"Apply Skin"** button
2. Wait for status message: "Success: Skin applied!"
3. In CS2, you need to:
   - Switch to a different weapon
   - Switch back to the modded weapon
   - OR drop the weapon (G key) and pick it up again

### 6. See Your Changes

The skin should now be visible on your weapon!

Note: 
- Changes are CLIENT-SIDE ONLY
- Other players will NOT see your skins
- Skins reset when you restart CS2

### 7. Reset (Optional)

To restore default skins:
1. Click **"Reset Skins"** button
2. Switch weapons or drop/pickup to see default skins again

## Tips and Tricks

### Best Practices

1. **Test in Practice Mode**
   - Use Practice with bots for testing
   - Enable `sv_cheats 1` for quick weapon spawning
   - Use `give weapon_ak47` to spawn specific weapons

2. **Quick Weapon Spawning**
   ```
   sv_cheats 1
   give weapon_ak47
   give weapon_awp
   give weapon_knife_karambit
   ```

3. **See Changes Faster**
   - Drop weapon (G key) and pick up
   - Use `r_drawothermodels 2` to see through walls
   - Use `mp_roundtime 60` for longer rounds

### Console Commands (Practice Mode)

```
// Enable cheats
sv_cheats 1

// Infinite ammo
sv_infinite_ammo 1

// No recoil
weapon_recoil_scale 0

// Buy anywhere
mp_buy_anywhere 1

// Spawn weapons
give weapon_ak47
give weapon_m4a1
give weapon_awp
give weapon_knife_butterfly
```

## Troubleshooting

### Problem: Can't connect to CS2

**Solution:**
- Ensure CS2 is fully loaded into a game
- Try clicking "Refresh Process" again after loading into a match
- Run the skin changer as Administrator
- Check Task Manager for `cs2.exe` process

### Problem: Skin doesn't appear

**Solution:**
- Drop the weapon and pick it up
- Switch to another weapon and back
- Restart the round (in practice mode: `mp_restartgame 1`)
- The skin data might not have updated visually

### Problem: "Failed to apply skin" error

**Solution:**
- Offsets may be outdated (check for updated `offsets.json`)
- Make sure you're holding the correct weapon
- Try reconnecting (click "Refresh Process")
- CS2 may have updated - need new offsets

### Problem: Application crashes

**Solution:**
- Ensure config files are present in `config/` directory
- Check Windows Event Viewer for error details
- Try running as Administrator
- Verify CS2 is actually running

### Problem: Changes disappear

**Solution:**
- Skins are memory-only modifications
- They reset when CS2 restarts
- Reapply skins after each CS2 restart
- This is expected behavior

## Advanced Usage

### Testing Multiple Skins Quickly

1. Keep CS2 in windowed mode
2. Keep skin changer open on second monitor
3. Use console commands to spawn weapons quickly:
   ```
   bind "F1" "give weapon_ak47"
   bind "F2" "give weapon_awp"
   bind "F3" "give weapon_knife_karambit"
   ```

### Creating Custom Skin Configurations

Edit `config/skins.json` to add your favorite skins:

```json
{
  "name": "Custom Skin Name",
  "paintKit": 1234
}
```

Find paint kit IDs from:
- CS2 game files
- Community databases
- Skin ID websites

## Safety Reminders

### DO:
- ✅ Use in offline practice mode
- ✅ Use with bots only
- ✅ Test on a secondary account
- ✅ Keep CS2 in offline mode
- ✅ Learn how the tool works

### DON'T:
- ❌ Use in online matchmaking
- ❌ Use in competitive games
- ❌ Use on your main account
- ❌ Use in community servers
- ❌ Expect to avoid VAC ban

## Understanding How It Works

The skin changer:
1. Finds CS2 process in memory
2. Locates `client.dll` module
3. Reads weapon structure addresses
4. Writes paint kit ID to weapon memory
5. Weapon appears different on YOUR screen only

This is why:
- Other players can't see your skins
- It works offline
- It's client-side only
- It's still detectable by VAC

## Getting Help

If you encounter issues:

1. **Check README.md** for troubleshooting
2. **Verify configuration files** are correct
3. **Update offsets** if CS2 has updated
4. **Test in practice mode** first

Remember: This is an educational tool. Using it will likely result in a VAC ban.

## Updating After CS2 Patches

When CS2 updates:
1. Game offsets change
2. Tool will stop working
3. Need updated `offsets.json`
4. Check community sources for updated offsets
5. Or use pattern scanning to find new offsets

Community offset sources:
- GitHub repositories
- CS2 modding forums
- Reverse engineering communities

---

**FINAL WARNING: Use at your own risk. VAC bans are permanent.**
