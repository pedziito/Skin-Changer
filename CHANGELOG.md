# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2024-01-01

### Added
- Initial release of CS2 Skin Changer
- Core memory manipulation functionality
  - ProcessManager for process attachment and memory operations
  - PatternScanner for dynamic address finding
  - ReadProcessMemory/WriteProcessMemory implementations
- Offset management system
  - JSON-based configuration
  - Easy offset updates for game patches
- Comprehensive skin database
  - Popular skins for Rifles, Pistols, Knives, and SMGs
  - Weapon categorization
  - Easy addition of new skins
- Windows native GUI application
  - Category/Weapon/Skin selection dropdowns
  - Apply and Reset functionality
  - Status messages and error handling
  - VAC warning prominently displayed
- SkinChanger core module
  - Weapon skin application
  - Paint kit, wear, seed, and StatTrak support
  - Safe memory modification with validation
- Comprehensive documentation
  - README with warnings and features
  - BUILD.md for build instructions
  - USAGE.md for user guide
  - CONTRIBUTING.md for contributors
  - SECURITY.md for security policy
  - LICENSE (MIT with disclaimers)
- Configuration files
  - offsets.json for memory offsets
  - skins.json for weapon and skin definitions
- CMake build system
  - Cross-platform build configuration
  - Visual Studio support
  - Automatic config file copying
- Safety features
  - Process validation (CS2 only)
  - Error handling throughout
  - Quick reset functionality

### Security
- All operations are client-side only
- No network manipulation
- No DLL injection
- Clear warnings about VAC detection

### Documentation
- Clear disclaimers about:
  - VAC ban risks
  - Terms of Service violations
  - Permanent ban consequences
  - Educational purpose only
- Detailed usage instructions
- Build and configuration guides
- Troubleshooting section

## [Unreleased]

### Planned Features
- Automatic offset updating
- Pattern-based offset finding
- More skins in database
- GUI improvements
- Better error messages
- Configuration validation

### Known Issues
- Offsets need manual updates after game patches
- Limited to currently equipped weapon
- Requires game restart to reset skins
- No persistent skin configurations

## Version History

### Version Numbering
- **Major (X.0.0)**: Breaking changes, major rewrites
- **Minor (1.X.0)**: New features, improvements
- **Patch (1.0.X)**: Bug fixes, small updates

## Future Releases

### 1.1.0 (Planned)
- Improved pattern scanner
- Auto-update check for offsets
- Enhanced GUI with search
- More weapon categories
- Configuration presets

### 1.2.0 (Planned)
- Alternative GUI framework option
- Theme customization
- Better logging system
- Performance improvements

## Compatibility Notes

### CS2 Updates
When CS2 updates, offsets typically change and the tool stops working until:
1. New offsets are found
2. Configuration is updated
3. Tool is rebuilt (if necessary)

### Windows Versions
- Windows 10 (1809+): Fully supported
- Windows 11: Fully supported
- Windows 8.1 and earlier: Not tested

### Visual Studio Versions
- VS 2019: Fully supported
- VS 2022: Fully supported
- VS 2017: May work, not officially supported

## Deprecation Notices

None currently.

## Breaking Changes

None currently (initial release).

---

For detailed commit history, see [GitHub Commits](https://github.com/pedziito/Skin-Changer/commits/).
