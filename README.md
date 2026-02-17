# CS2 Skin Changer

[![GitHub release](https://img.shields.io/github/v/release/pedziito/Skin-Changer?style=flat-square)](https://github.com/pedziito/Skin-Changer/releases)
[![License](https://img.shields.io/badge/license-MIT-blue?style=flat-square)](https://github.com/pedziito/Skin-Changer/blob/main/LICENSE)
[![Windows](https://img.shields.io/badge/platform-Windows%2064bit-brightgreen?style=flat-square)](https://github.com/pedziito/Skin-Changer)

A window-based skin changer for Counter-Strike 2 featuring a .exe loader with .dll injection and in-game menu overlay.

## ⚠️ **CRITICAL: VAC BAN WARNING** ⚠️

**THIS TOOL WILL RESULT IN A PERMANENT BAN**

- **VAC DETECTION**: This tool is almost certainly detectable by VAC (Valve Anti-Cheat)
- **PERMANENT BAN**: VAC bans cannot be appealed and are permanent
- **ACCOUNT LOSS**: All games and items on your Steam account will be lost
- **IP BLOCKING**: Your IP may be blocked from playing VAC-secured games
- **FOR EDUCATIONAL USE ONLY**: This is a proof-of-concept, not for actual gameplay

**DO NOT USE ON YOUR MAIN ACCOUNT**

The developers assume NO responsibility for bans or account loss. Use entirely at your own risk.

---

## Features

### Loader Application (cs2loader.exe)
- 🎮 Automatic CS2 process detection
- 📦 Handles DLL injection reliably
- 🔐 License verification
- ⚠️ VAC warning display
- 🔄 Automatic dependency management

### In-Game Overlay Menu
- 🎨 Clean, intuitive menu interface (INS key to toggle)
- 🎯 Real-time skin selection
- 🔧 Multiple configuration options
- 📊 Game memory integration
- ⌨️ Keyboard-based navigation

### License System
- 🔐 User licensing requirements
- ⏰ Time-based license expiry
- 🛡️ License revocation support
- 👨‍💼 Admin tool for license management

## Installation

### Requirements
- Windows 7 or later (x64)
- GPU that supports DirectX 11
- Decent CPU for injection and overlay

### Download
Download the latest release from [GitHub Releases](https://github.com/pedziito/Skin-Changer/releases)

Extract files:
```
CS2Changer/
├── cs2loader.exe       (Main application)
├── CS2Changer.dll      (Game hook)
└── license.key         (User license)
```

### Setup

1. **Obtain License**
   - Contact admin at: [your-contact-info]
   - You'll receive a `license.key` file

2. **Place license.key**
   - Copy `license.key` to the same folder as `cs2loader.exe`
   - Ensures loader can verify your license

3. **Run Loader**
   ```
   cs2loader.exe
   ```
   - Accept VAC warning
   - Loader will wait for CS2 to start

4. **Launch Counter-Strike 2**
   - Start the game normally
   - Loader automatically injects DLL
   - Menu becomes available in-game

5. **Use In-Game Menu**
   - Press `INS` key to toggle menu
   - Navigate with arrow keys
   - Press `ENTER` to select
   - Choose weapon → Choose skin → Apply

## Admin Panel (License Management)

### Launch Admin Tool
```
cs2admin.exe
```

### Admin Capabilities
- Generate new licenses with expiry dates
- Validate existing license files
- Revoke compromised licenses
- Set custom usernames per license

### License Format
```
CS2-2026-A1B2C3D4-02-E5F6G7H8
USERNAME=player_name
CREATED=2026-02-17 12:34:56
EXPIRY=2026-03-19 12:34:56
ACTIVE=true
```

## Building from Source

### Requirements
- CMake 3.15+
- Visual Studio 2019+ (MSVC compiler)
- Windows SDK 10.0+
- C++17 or later

### Build Steps

```bash
# Clone repository
git clone https://github.com/pedziito/Skin-Changer.git
cd Skin-Changer

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -G "Visual Studio 16 2019"

# Build Release
cmake --build . --config Release

# Built files in: build/bin/
# - cs2loader.exe
# - CS2Changer.dll
# - cs2admin.exe
```

## Architecture

The application consists of three main components:

1. **Loader (cs2loader.exe)**
   - Displays VAC warning
   - Verifies license
   - Waits for CS2 process
   - Injects DLL using CreateRemoteThread

2. **Game DLL (CS2Changer.dll)**
   - Injected into CS2 process
   - Provides in-game overlay menu
   - Interfaces with game memory
   - Applies skin modifications

3. **Admin Tool (cs2admin.exe)**
   - Password-protected license management
   - Generate licenses with expiration
   - Validate and revoke licenses
   - User administration

For detailed architecture information, see [ARCHITECTURE.md](ARCHITECTURE.md).

## Configuration

No manual configuration required! The loader uses:
- Auto-detection of CS2 process
- License file validation
- Built-in skin database
- Automatic menu initialization

## Troubleshooting

### "DLL not found"
- Ensure `CS2Changer.dll` is in the same folder as `cs2loader.exe`

### "License file not found"
- Place `license.key` in the application directory
- Contact admin for license file

### Menu doesn't appear
- Press INS key to toggle visibility
- Ensure DLL was injected successfully
- Check Windows UAC settings

### DLL injection fails
- Run as Administrator
- Disable compatibility mode
- Try different CS2 versions
- Check antivirus software restrictions

## FAQ

### Is this detectable?
Yes. VAC detection is very likely.

### Will I get banned?
Very likely. Do not use on accounts you care about.

### Is it free?
License keys are generated by admins only.

### Can I modify it?
Yes, source code is available. See [CONTRIBUTING.md](CONTRIBUTING.md)

## Security Notes

- License keys contain embedded expiry information
- Admin password is hardcoded (production: use external auth)
- DLL injection uses standard Windows API
- No external network calls (offline compatible)
- Automatic VAC warning keeps users informed

## Project Structure

```
Skin-Changer/
├── loader/              # .exe loader
├── injector/            # .dll game hook
├── admin/               # License management CLI
├── src/                 # Core game logic
├── include/             # Header files
├── config/              # Skin database
└── CMakeLists.txt       # Build configuration
```

## Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md) - Technical architecture details
- [HOW_TO_OPEN.md](HOW_TO_OPEN.md) - IDE setup guide
- [BUILD.md](BUILD.md) - Detailed build instructions
- [QUICKSTART.md](QUICKSTART.md) - Quick start guide

## Disclaimer

**This tool is provided "AS IS" without any warranty.** The authors and contributors are not responsible for any consequences, including but not limited to:

- VAC bans
- Account suspensions
- Loss of games or items
- IP bans
- Loss of access to Steam services
- Any other penalties imposed by Valve

**Use entirely at your own risk.**

## License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file for details.

This project is for **EDUCATIONAL PURPOSES ONLY**. Unauthorized modification of commercial games may violate laws in your jurisdiction.

## Author

- **pedziito** - Project creator and maintainer

## Acknowledgments

- Counter-Strike 2 community
- Game modifier enthusiasts
- Open-source contributors
- User-friendly Windows GUI interface
- Weapon categorization (Rifles, Pistols, Knives, SMGs)
- Extensive skin database with popular skins
- Configurable offsets for game updates
- Safe process validation
- Quick reset functionality
- Web API synchronization support

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

## Quick Start

### Option 1: Web Dashboard (Recommended)

1. **Start the web server**:
   ```bash
   cd web/backend
   npm install
   npm start
   ```

2. **Open your browser**: Navigate to `http://localhost:3000`

3. **Create an account**: Sign up with username, email, and password

4. **Configure skins**: Select weapon categories, weapons, and skins through the web interface

5. **Generate API token**: Click "Generate Token" to create a token for the desktop client

6. **Use desktop client** (optional): Configure the .exe with your API token to sync settings

### Option 2: Desktop Application Only

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

