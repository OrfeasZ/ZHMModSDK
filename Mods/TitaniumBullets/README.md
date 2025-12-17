# TitaniumBullets - ZHMModSDK Plugin

A ZHMModSDK plugin for HITMAN 3 that enables bullets to penetrate through walls while still hitting NPCs.

## Purpose

This plugin mirrors the Simple Mod Framework "TitaniumBullets" mod but runs as a native ZHMModSDK plugin. Useful when:

- You want to use WeMod (or other trainers) alongside wall penetration
- The SMF version crashes when combined with other tools
- You prefer runtime modification instead of file-based modding

## How It Works

The plugin patches the in-memory repository (`[assembly:/repository/pro.repo].pc_repo`):

- For the same repository IDs as SMF's `TitaniumBullets.repository.json`, it overrides `AmmoConfig` to the penetration config (`87ae0524-2f22-4fe0-82e1-84a050b43cf0`)
- When disabled, it restores the original `AmmoConfig` values

## Installation

### Pre-built (if available)
1. Copy `TitaniumBullets.dll` to your HITMAN 3 installation folder: `Retail/mods/`
2. Make sure ZHMModSDK is properly installed

### Building from Source

#### Prerequisites
- Visual Studio 2022 (Desktop C++ workload)
- CMake 3.15 or higher
- vcpkg (included as a submodule)
- Git

#### Build Steps

1. **Clone the fork (with submodules)**  
   ```bash
   git clone --recursive https://github.com/Prekzursil/ZHMModSDK.git
   cd ZHMModSDK
   ```

2. **Bootstrap vcpkg (first time only)**  
   ```powershell
   .\External\vcpkg\bootstrap-vcpkg.bat
   ```

3. **Configure** (optional: set `GAME_INSTALL_PATH` to your Hitman 3 folder containing `Retail`)  
   ```powershell
   cmake --preset x64-Release . -DGAME_INSTALL_PATH="C:/Program Files/Epic Games/HITMAN3"
   ```

4. **Build just this mod**  
   ```powershell
   cmake --build _build/x64-Release --target TitaniumBullets
   ```

5. **Install**  
   ```powershell
   cmake --install _build/x64-Release
   ```

## Usage

1. Launch HITMAN 3 with ZHMModSDK
2. Open the SDK menu (default: `~` key)
3. Look for the **"Titanium Bullets"** checkbox
4. Enable/disable as needed

The setting is saved automatically and persists between sessions.

## Compatibility

| Software | Compatible | Notes |
|----------|------------|-------|
| ZHMModSDK | Yes | Required base |
| WeMod | Yes | Works together |
| Cheat Engine | Yes | No known conflicts |
| Simple Mod Framework | Not needed | Use this plugin instead of SMF's TitaniumBullets |
| Other ZHMModSDK mods | Yes | Works alongside |

## Troubleshooting

### Bullets still not penetrating walls
- Ensure the mod is enabled (check the SDK menu)
- If the SDK log says "No matching repository entries found to patch", the target repository IDs likely changed with a game update

### Game crashes
- Ensure ZHMModSDK is up to date
- Check that the mod DLL is in the correct `mods` folder
- Disable other mods to isolate the issue

### Hook not installing
- The collision manager may not be available yet when the mod loads
- Try loading into a mission (the hook installs on engine initialization)

## Technical Details

- Loads `pro.repo` via `Globals::ResourceManager` and edits `ZDynamicObject` entries to replace `AmmoConfig`
- Restores original values when disabled or on scene unload

### Files
- `TitaniumBullets.h` - Plugin header
- `TitaniumBullets.cpp` - Main implementation (repository patch + restore)
- `CMakeLists.txt` - Build configuration

## License

This plugin is provided as-is for educational and personal use. Use at your own risk.

## Credits

- ZHMModSDK team for the modding framework
- HITMAN modding community for research and documentation
