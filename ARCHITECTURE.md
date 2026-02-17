# CS2 Skin Changer - Application Architecture

## Overview

CS2 Skin Changer is a game modification tool for Counter-Strike 2. This document describes the new architecture after transitioning from the web-based system to a traditional Windows application with DLL injection.

## Architecture

### Components

#### 1. **Loader (cs2loader.exe)**
- Entry point for the user
- Displays VAC warning before loading
- Verifies license validity
- Monitors for CS2 process
- Injects the DLL into the game
- Non-intrusive: user can close loader after DLL injection

**Location:** `/loader/main.cpp`

#### 2. **DLL Injector (CS2Changer.dll)**
- Injected into CS2 process memory
- Provides in-game overlay menu
- Communicates with game memory
- Implements skin modification logic
- Licensed: requires valid `license.key` to load

**Location:** `/injector/GameMenu.cpp`, `/injector/dllmain.cpp`

#### 3. **Core Library (CS2Core.lib)**
- Shared game interaction logic
- Memory scanning and pattern matching
- Skin database management
- Used by both loader and DLL

**Location:** `/src/`, `/include/`

#### 4. **Admin Tool (cs2admin.exe)**
- CLI for license management
- Generate new licenses with expiry dates
- Validate existing licenses
- Revoke licenses
- Password protected (admin only)

**Location:** `/admin/main.cpp`

## Build System

Uses **CMake** to build all components:

```
CMakeLists.txt
├── Loader (cs2loader.exe)
├── DLL (CS2Changer.dll)
├── Core Library (CS2Core.lib)
└── Admin Tool (cs2admin.exe)
```

## Workflow

### User Workflow

1. **Obtain License**
   - Contact admin with unique identifier
   - Receive `license.key` file

2. **Launch Application**
   ```
   cs2loader.exe
   ```

3. **See VAC Warning**
   - Informed of risk
   - Must acknowledge to continue

4. **Start Counter-Strike 2**
   - Loader waits for CS2 process
   - Detects game launch
   - Injects DLL

5. **Use In-Game Menu**
   - Press `INS` key to toggle menu
   - Select weapon
   - Choose skin
   - Apply changes

### Admin Workflow

1. **Launch Admin Tool**
   ```
   cs2admin.exe
   ```

2. **Enter Admin Password**
   - Currently: `Mao770609`

3. **Manage Licenses**
   - Generate new licenses
   - Set expiry dates
   - Validate licenses
   - Revoke compromised licenses

## File Structure

```
Skin-Changer/
├── loader/              # .exe loader application
│   └── main.cpp
├── injector/            # .dll game hook
│   ├── GameMenu.h
│   ├── GameMenu.cpp
│   └── dllmain.cpp
├── admin/               # License management tool
│   └── main.cpp
├── src/                 # Core game modification logic
│   ├── ProcessManager.cpp
│   ├── SkinChanger.cpp
│   ├── ...
│   └── main.cpp (legacy, not used)
├── include/             # Header files
│   ├── ProcessManager.h
│   ├── SkinChanger.h
│   └── ...
└── CMakeLists.txt       # Build configuration
```

## In-Game Menu

The DLL provides a text-based overlay menu (planned ImGui integration):

```
┌─────────────────────────────────┐
│  CS2 SKIN CHANGER - MAIN MENU   │
├─────────────────────────────────┤
│  [ ] Change Skins               │
│  [ ] Settings                   │
│  [ ] About                      │
│  [ ] Exit                       │
└─────────────────────────────────┘
```

### Menu Navigation
- `UP/DOWN ARROWS`: Navigate menu items
- `LEFT/RIGHT ARROWS`: Change values
- `ENTER`: Select option
- `ESC` / `INS`: Toggle menu visibility

## License System

### License Format

```
CS2-2026-A1B2C3D4-02-E5F6G7H8
USERNAME=player_name
CREATED=2026-02-17 12:34:56
EXPIRY=2026-03-19 12:34:56
ACTIVE=true
```

### License Validation

- Checked when loader starts
- Must exist in application directory
- Format validation required
- Expiry date verification
- Can be revoked by admin

## Security Features

### VAC Warning
- Displays before DLL injection
- User acknowledgment required
- Warns about ban consequences
- In-game reminder available

### License Verification
- Prevents unauthorized access
- Time-limited licenses
- Admin revocation capability
- Unique per user

### Memory Protection
- DLL injection verification
- License key validation
- Anti-crack mechanisms

## Building

### Requirements
- CMake 3.15+
- Visual Studio 2019+
- Windows SDK
- C++17 compiler

### Build Commands

```bash
# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build . --config Release

# Outputs
bin/cs2loader.exe
bin/CS2Changer.dll
bin/cs2admin.exe
```

## Known Limitations

- Menu is currently text-based (ImGui overlay coming soon)
- Requires admin privileges for injection
- Windows only (x64)
- Requires license to operate
- VAC/EAC may detect modification

## Future Improvements

- [ ] DirectX/ImGui overlay rendering
- [ ] Advanced menu navigation system
- [ ] Config file save/load
- [ ] Statistics tracking
- [ ] Update system
- [ ] Multi-account management
- [ ] Automatic database updates

## Support

For issues or license generation, contact admin.

---

*Last Updated: February 17, 2026*
