# Building CS2 Skin Changer

## Prerequisites

### Required Software

1. **CMake** (version 3.15 or higher)
   - Download from: https://cmake.org/download/
   - Add to PATH during installation

2. **Visual Studio 2019 or 2022**
   - Download Community Edition: https://visualstudio.microsoft.com/
   - Required workloads:
     - Desktop development with C++
     - Windows 10/11 SDK

3. **Git** (optional, for cloning)
   - Download from: https://git-scm.com/

## Building Steps

### Method 1: Command Line (Recommended)

1. **Open Developer Command Prompt for VS**
   - Start Menu → Visual Studio → Developer Command Prompt

2. **Navigate to project directory**
   ```bash
   cd C:\path\to\Skin-Changer
   ```

3. **Create and enter build directory**
   ```bash
   mkdir build
   cd build
   ```

4. **Generate project files**
   ```bash
   # For Visual Studio 2019
   cmake .. -G "Visual Studio 16 2019" -A x64
   
   # For Visual Studio 2022
   cmake .. -G "Visual Studio 17 2022" -A x64
   ```

5. **Build the project**
   ```bash
   # Debug build
   cmake --build . --config Debug
   
   # Release build (recommended)
   cmake --build . --config Release
   ```

6. **Find the executable**
   - Debug: `build/bin/Debug/CS2SkinChanger.exe`
   - Release: `build/bin/Release/CS2SkinChanger.exe`

### Method 2: Visual Studio IDE

1. **Generate Visual Studio Solution**
   ```bash
   mkdir build
   cd build
   cmake .. -G "Visual Studio 16 2019" -A x64
   ```

2. **Open the Solution**
   - Open `build/CS2SkinChanger.sln` in Visual Studio

3. **Build**
   - Select "Release" configuration from dropdown
   - Build → Build Solution (or press Ctrl+Shift+B)

4. **Run**
   - Press F5 to run with debugging
   - Or find executable in `build/bin/Release/`

## Configuration Files

After building, copy configuration files to the executable directory:

```bash
# If running from build/bin/Release/
mkdir config
copy ..\..\..\..\config\offsets.json config\
copy ..\..\..\..\config\skins.json config\
```

Or the CMake configuration should handle this automatically.

## Troubleshooting

### CMake not found
- Ensure CMake is installed and added to PATH
- Restart your terminal/command prompt after installation

### Visual Studio not found
- Specify the generator explicitly:
  ```bash
  cmake .. -G "Visual Studio 16 2019" -A x64
  ```
- Make sure Visual Studio with C++ tools is installed

### Link errors
- Ensure you have Windows SDK installed
- Try cleaning and rebuilding:
  ```bash
  cmake --build . --target clean
  cmake --build . --config Release
  ```

### Missing config files
- Make sure `config/offsets.json` and `config/skins.json` exist
- Copy them manually to the executable directory if needed

## Clean Build

To start fresh:

```bash
# Remove build directory
rmdir /s build

# Recreate and build
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

## Advanced Build Options

### Custom Install Location

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=C:/MyTools
cmake --build . --config Release --target install
```

### Debug Build with Symbols

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
```

## Next Steps

After successful build:
1. Navigate to `build/bin/Release/`
2. Ensure `config/` directory with JSON files is present
3. Read [USAGE.md](USAGE.md) for usage instructions
4. Review [README.md](README.md) for important warnings
