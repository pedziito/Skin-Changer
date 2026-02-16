# CS2 Skin Changer

A client-side skin changer for Counter-Strike 2 that modifies weapon skin appearances locally on the user's machine.

## ⚠️ IMPORTANT DISCLAIMERS

**READ THIS BEFORE PROCEEDING:**

- **This tool violates Valve's Terms of Service and Steam Subscriber Agreement**
- **Using this tool WILL result in a permanent VAC (Valve Anti-Cheat) ban**
- **VAC bans are permanent and irreversible**
- **VAC bans affect ALL VAC-secured games on your Steam account**
- **This project is for EDUCATIONAL PURPOSES ONLY**
- **Use at your own risk - you assume ALL responsibility**

The developers of this tool are not responsible for any bans, account suspensions, or other consequences resulting from its use.

## Features

- Client-side weapon skin modification
- Real-time skin application while game is running
- User-friendly Windows GUI interface
- Weapon categorization (Rifles, Pistols, Knives, SMGs)
- Extensive skin database with popular skins
- Configurable offsets for game updates
- Safe process validation
- Quick reset functionality

## Technical Architecture

### Components

1. **Main Application (GUI)**: Windows native GUI for user interaction
2. **Process Manager**: Handles process attachment and memory operations
3. **Pattern Scanner**: Dynamic address finding using byte patterns
4. **Offset Manager**: Manages and updates memory offsets
5. **Skin Database**: Stores weapon and skin definitions
6. **Skin Changer**: Core logic for applying skin modifications

### How It Works

The application:
1. Attaches to the CS2 process using Windows API
2. Locates the `client.dll` module
3. Uses pattern scanning or static offsets to find game structures
4. Writes skin paint kit values directly to weapon memory
5. Modifications are client-side only and not visible to other players

## Build Requirements

- Windows 10/11
- CMake 3.15 or higher
- Visual Studio 2019 or higher (with C++ toolset)
- Windows SDK

## Building from Source

### Using CMake

```bash
# Create build directory
mkdir build
cd build

# Generate Visual Studio project
cmake .. -G "Visual Studio 16 2019" -A x64

# Build the project
cmake --build . --config Release
```

### Using Visual Studio

1. Open the generated `.sln` file in the build directory
2. Select "Release" configuration
3. Build Solution (Ctrl+Shift+B)

The executable will be in `build/bin/Release/CS2SkinChanger.exe`

## Usage

1. **Start CS2**: Launch Counter-Strike 2 and load into a game or practice mode
2. **Run the Tool**: Execute `CS2SkinChanger.exe` (as Administrator may be required)
3. **Connect to Process**: Click "Refresh Process" to attach to CS2
4. **Select Skin**:
   - Choose a weapon category (Rifles, Pistols, Knives, SMGs)
   - Select a weapon
   - Pick your desired skin
5. **Apply**: Click "Apply Skin" to modify the weapon
6. **In-Game**: Switch to the weapon or drop and pick it up to see changes
7. **Reset**: Use "Reset Skins" to restore default skins

## Configuration Files

### offsets.json

Contains memory offsets for CS2. Update these after game patches:

```json
{
  "version": "1.0.0",
  "offsets": {
    "dwLocalPlayerPawn": "0x17370B8",
    "dwEntityList": "0x18BBDC8",
    "m_pClippingWeapon": "0x1308",
    ...
  },
  "patterns": {
    "dwLocalPlayerPawn": "48 8B 05 ? ? ? ? 48 85 C0 74 ? 8B 88",
    ...
  }
}
```

### skins.json

Defines available weapons and skins with their paint kit IDs:

```json
{
  "categories": {
    "Rifles": {
      "AK-47": {
        "id": 7,
        "skins": [
          {"name": "Redline", "paintKit": 282},
          ...
        ]
      }
    }
  }
}
```

## Updating Offsets

After CS2 updates, offsets may change. To update:

1. Use a memory dumper tool to find new offsets
2. Use pattern scanning tools (like Cheat Engine)
3. Update `config/offsets.json` with new values
4. Community offset databases (e.g., hazedumper)

## Safety Features

- **Process Validation**: Only attaches to CS2 process
- **Error Handling**: Comprehensive error checking for memory operations
- **Quick Disable**: Reset button to restore original values
- **No Network Manipulation**: Purely client-side modifications

## Limitations

- **Client-side only**: Changes are not visible to other players
- **VAC Detection**: High risk of detection and ban
- **Update Dependent**: Requires offset updates after game patches
- **Online Use**: DO NOT use in online matchmaking
- **Local/Offline Only**: Intended for offline/local testing only

## Troubleshooting

### "Failed to find CS2 process"
- Make sure CS2 is running
- Try running the tool as Administrator
- Verify CS2 process name is `cs2.exe`

### "Failed to find client.dll"
- CS2 may not be fully loaded
- Wait until you're in a game/practice mode
- Try clicking "Refresh Process" again

### "Failed to apply skin"
- Offsets may be outdated after a game update
- Check for updated offset configurations
- Verify you're holding the correct weapon

### Changes not visible
- Drop and pick up the weapon
- Switch to another weapon and back
- Restart the round (in practice mode)

## Development

### Project Structure

```
Skin-Changer/
├── CMakeLists.txt
├── README.md
├── config/
│   ├── offsets.json
│   └── skins.json
├── include/
│   ├── GUI.h
│   ├── OffsetManager.h
│   ├── PatternScanner.h
│   ├── ProcessManager.h
│   ├── SkinChanger.h
│   └── SkinDatabase.h
└── src/
    ├── GUI.cpp
    ├── OffsetManager.cpp
    ├── PatternScanner.cpp
    ├── ProcessManager.cpp
    ├── SkinChanger.cpp
    ├── SkinDatabase.cpp
    └── main.cpp
```

### Adding New Skins

Edit `config/skins.json` and add entries:

```json
{
  "name": "New Skin Name",
  "paintKit": 1234
}
```

Paint kit IDs can be found in game files or community databases.

## Legal Notice

This software is provided for educational purposes only. The developers:
- Do not condone cheating or violating game Terms of Service
- Are not responsible for any consequences of using this software
- Do not provide support for ban appeals or account recovery
- Recommend using this only in offline/local environments for learning

**By using this software, you acknowledge that you understand the risks and accept full responsibility for any consequences.**

## Contributing

This is an educational project. Contributions should focus on:
- Code quality improvements
- Documentation enhancements
- Bug fixes
- Security improvements

Do not submit contributions that:
- Encourage online/competitive use
- Bypass additional anti-cheat measures
- Promote malicious use

## License

This project is provided as-is for educational purposes. Use at your own risk.

## Resources

- [CS2 Game Updates](https://store.steampowered.com/news/app/730)
- [Valve Anti-Cheat (VAC) System](https://help.steampowered.com/en/faqs/view/571A-97DA-70E9-FF74)
- [Steam Subscriber Agreement](https://store.steampowered.com/subscriber_agreement/)

## Credits

This tool was created for educational purposes to demonstrate:
- Windows API process manipulation
- Memory reading/writing techniques
- Pattern scanning algorithms
- Client-side game modification concepts

---

**Remember: This is a learning tool. Using it in online games will result in a permanent ban.**

