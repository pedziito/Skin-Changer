# Quick Start Guide

**⚠️ WARNING: Using this tool will result in a VAC ban. Use only for educational purposes on a test account.**

## Prerequisites

- Windows 10 or 11 (x64)
- Counter-Strike 2 installed
- Test Steam account (expect it to be banned)
- Administrator privileges
- Valid CS2 license key (for initial sign-up, format: CS2-*)

## Quick Setup (2 Minutes)

### Step 1: Download

Download `cs2inventory.exe` from [releases](https://github.com/pedziito/Skin-Changer/releases) and save it to a folder.

The executable is completely standalone - no additional files or configuration needed!

### Step 2: First Run - Sign Up

Run the executable with administrator privileges:

```
1. Double-click cs2inventory.exe
2. Console window opens showing:
   ┌─────────────────────────────────┐
   │  CS2 INVENTORY CHANGER v1.0    │
   └─────────────────────────────────┘
   
3. Enter your CS2 license key (example: CS2-1234-5678-9012)
4. Choose a username
5. Set a password (input hidden with *)
6. Confirm password
7. System captures your Hardware ID (HWID) for device locking
8. Success message displays
```

### Step 3: Subsequent Runs - Login

On future runs, the application will:

```
1. Detect existing account configuration
2. Prompt for login:
   - Username
   - Password
3. Verify Hardware ID matches
4. If HWID differs (different PC), login will fail
   → Contact support to reset HWID to new device
```

### Step 4: Auto-Launch & Injection

After successful authentication:

```
1. Application automatically launches Steam
2. Waits for Steam initialization
3. Launches Counter-Strike 2
4. Monitors for CS2 process
5. Injects inventory changer DLL
6. Displays success message
7. Console window closes
```

### Step 5: In-Game Menu

While in Counter-Strike 2:

```
1. Press INS (Insert key)
2. Overlay menu appears
3. Navigate with arrow keys / WASD
4. Modify inventory items
5. Press ESC to close menu
```

## Build from Source (Windows)

### Step 1: Clone Repository

```bash
git clone https://github.com/pedziito/Skin-Changer.git
cd Skin-Changer
```

### Step 2: Build with Visual Studio

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

**Note**: Requires cmake 3.15+ and Visual Studio 2022 installed

### Step 3: Find Executable

Built executable will be at:
```
build\bin\Release\cs2inventory.exe
```

### Step 4: Test

Run with administrator privileges and follow the sign-up/login flow above.

## Troubleshooting

### "License must start with 'CS2-'"
- Your license key format is invalid
- Use valid CS2 license key starting with CS2-
- Example: CS2-ABC123-DEF456-GHI789

### "HWID mismatch!"
- You're trying to login on a different computer
- Original device: Contact support for HWID reset
- This is intentional for preventing account sharing

### "CS2 process not found"
- CS2 failed to launch properly
- Ensure you have administrator privileges
- Check that Counter-Strike 2 is installed
- Start the game manually and run injector again

### "Injection failed"
- Missing administrator privileges
- Run executable as Administrator (right-click → Run as Administrator)
- Anti-virus may be blocking injection

## Configuration

The application stores your credentials in:
```
user_config.txt
```

Located in the same directory as `cs2inventory.exe`. 

**Never share this file** - it contains your login credentials and HWID.
2. Go to Practice → Practice Mode with bots
3. Start a game
4. Wait until fully loaded in-game

### Step 4: Run Skin Changer

1. Run `CS2SkinChanger.exe` (as Administrator if needed)
2. You'll see the warning message - acknowledge the risks
3. Click **"Refresh Process"** button
4. Wait for "Success: Connected to CS2 process"

### Step 5: Apply a Skin

1. Select **Category**: Rifles
2. Select **Weapon**: AK-47
3. Select **Skin**: Vulcan
4. Click **"Apply Skin"**
5. Status shows: "Success: Skin applied!"

### Step 6: See Changes In-Game

In CS2:
1. Open console (` or ~ key)
2. Type: `give weapon_ak47`
3. Your AK-47 should now have the Vulcan skin!

If you don't see changes:
- Drop weapon (G key) and pick it up
- Switch to another weapon and back
- Type `mp_restartgame 1` in console

## Console Commands Cheat Sheet

```cs
// Enable cheats (required for practice mode commands)
sv_cheats 1

// Spawn weapons
give weapon_ak47
give weapon_awp
give weapon_m4a1
give weapon_knife_karambit

// Infinite ammo
sv_infinite_ammo 1

// Restart game
mp_restartgame 1

// Restart round
mp_restartround 1

// Buy anywhere
mp_buy_anywhere 1

// Long round time (60 minutes)
mp_roundtime 60

// Bot commands
bot_add
bot_kick
bot_stop 1
```

## Common First-Time Issues

### Issue: "Failed to find CS2 process"
**Fix:** Ensure CS2 is running and fully loaded into a game (not just menu)

### Issue: "Failed to find client.dll"
**Fix:** Wait 30 seconds after joining a game, then click "Refresh Process" again

### Issue: Skin doesn't appear
**Fix:** Drop weapon (G) and pick it back up, or switch weapons

### Issue: Application won't start
**Fix:** Run as Administrator, check for config/ folder with JSON files

### Issue: Crashes immediately
**Fix:** Verify config files exist and are valid JSON

## Your First Skin Changes

### Try These Popular Skins

**Rifles:**
- AK-47: Redline, Vulcan, Asiimov
- AWP: Dragon Lore, Asiimov
- M4A4: Howl, Asiimov

**Pistols:**
- Desert Eagle: Blaze, Printstream
- Glock-18: Fade
- USP-S: Kill Confirmed

**Knives:**
- Karambit: Fade, Doppler
- Butterfly: Fade, Tiger Tooth

## Testing Multiple Skins

```cs
// In CS2 console:
sv_cheats 1
give weapon_ak47
// Apply AK-47 skin in tool

give weapon_awp
// Apply AWP skin in tool

give weapon_knife_karambit
// Apply knife skin in tool
```

## Practice Mode Setup

For best testing experience:

```cs
// Set up practice mode
sv_cheats 1
mp_warmup_end
mp_roundtime 60
mp_buytime 60000
mp_buy_anywhere 1
sv_infinite_ammo 1
mp_autoteambalance 0
mp_limitteams 0

// Add bots (optional)
bot_add_t
bot_add_ct
bot_stop 1

// Give yourself money
mp_maxmoney 65535
mp_startmoney 65535
```

## Reset Everything

To reset skins:
1. Click **"Reset Skins"** button in tool
2. Or restart CS2 (automatic reset)

## What NOT to Do

❌ Don't use in online matchmaking
❌ Don't use in competitive mode
❌ Don't use on your main account
❌ Don't expect to avoid VAC ban
❌ Don't use in community servers (unless you own them)

## What You CAN Do

✅ Use in offline practice mode
✅ Use on a test account
✅ Test different skins
✅ Learn how it works
✅ Understand the code
✅ Take screenshots for fun

## Next Steps

### Learn More
- Read [USAGE.md](USAGE.md) for detailed instructions
- Check [FAQ.md](FAQ.md) for common questions
- Review [README.md](README.md) for full documentation

### Customize
- Edit `config/skins.json` to add your favorite skins
- Update `config/offsets.json` when CS2 patches
- Modify the code to learn more

### Stay Safe
- Only use offline
- Use test accounts only
- Understand the risks
- Accept the consequences

## Pro Tips

1. **Bind Keys for Quick Weapon Spawning**
   ```cs
   bind "F1" "give weapon_ak47"
   bind "F2" "give weapon_awp"
   bind "F3" "give weapon_m4a1"
   ```

2. **Keep Tool Open While Playing**
   - Run CS2 in windowed mode
   - Keep skin changer on side
   - Apply skins on-the-fly

3. **Test Wear Values**
   - Current implementation uses 0.0001 (Factory New)
   - Edit code to test different wear values
   - 0.0 = FN, 0.07 = MW, 0.15 = FT, etc.

4. **Take Screenshots**
   - Before CS2 detects anything
   - Use F12 (Steam screenshot)
   - Or Windows + PrintScreen

## Timing Expectations

- **First launch:** 2-3 minutes (including CS2 startup)
- **Connecting tool:** 5-10 seconds
- **Applying skin:** Instant
- **Seeing changes:** 5-10 seconds (after drop/pickup)
- **Before VAC ban:** Hours to days (or never if only offline)

## Recommended Workflow

1. **Launch CS2** → Practice Mode
2. **Load map** → de_dust2 or de_mirage
3. **Open console** → Enable cheats
4. **Run tool** → Connect to process
5. **Spawn weapon** → Use give command
6. **Apply skin** → Select and apply in tool
7. **Verify** → Drop/pickup weapon
8. **Repeat** → Try different skins

## Emergency Stop

If something goes wrong:
1. Close CS2 (Alt+F4 or Task Manager)
2. Close skin changer tool
3. Restart CS2 (skins will be reset)
4. Everything returns to normal

## Remember

This is a **learning tool**. The goal is to:
- Understand Windows API
- Learn memory manipulation
- Study pattern scanning
- Explore game architecture
- Practice C++ programming

**NOT to**:
- Cheat in online games
- Gain unfair advantages
- Violate others' experience
- Risk your main account

---

## Support

Having trouble? Check:
1. [README.md](README.md) - Full documentation
2. [FAQ.md](FAQ.md) - Common questions
3. [USAGE.md](USAGE.md) - Detailed usage guide
4. [BUILD.md](BUILD.md) - Build instructions
5. GitHub Issues - Technical problems

**Remember: VAC bans are permanent. Use at your own risk.**

Happy learning! 🎓 (Not cheating! ⚠️)
