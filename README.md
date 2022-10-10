
# ZHM Mod SDK

A modding SDK and mod loader for HITMAN 3.

![build status](https://github.com/OrfeasZ/ZHMModSDK/workflows/Build/badge.svg)

## Description

This is a community-made modding SDK and mod loader for HITMAN 3. Its purpose is to allow users to easily download and use mods, and make the creation of more complex runtime mods easier for developers.

Right now it's at a very early stage, with only a few aspects of the engine exposed, but the plan is to further extend it to support basic entity, AI, and item manipulation.

## Usage (mod loader & mods)

1. Download the latest version of the mod loader and the mods by going [here](https://github.com/OrfeasZ/ZHMModSDK/releases/latest), and downloading `ZHMModSDK-Release.zip`. This zip file contains the mod loader and a few sample mods.

2. Extract the contents of the `ZHMModSDK-Release.zip` archive to `<drive>:\Path\To\HITMAN3\Retail`, where `<drive>:\Path\To\HITMAN3` is the path to your Hitman 3 installation directory. This will be at `C:\Program Files\EpicGames\HITMAN3` or `C:\Program Files (x86)\Steam\steamapps\common\HITMAN 3` by default. Make sure that you extract the files in the `Retail` folder and not the root `HITMAN3` folder.

3. Run the game like you normally would.

4. When the game opens, you'll see a dialog asking you which mods you'd like to use. Select them and press OK. You might need to restart your game for some of them to work.

5. ...

6. Profit?

> NOTE: You can at any time change the mods you're using by pressing the `~` key and selecting them, or by editing the `mods.ini` file inside your game's `Retail` folder.

## Sample mods

There are a few sample mods included in this repository that can be used either as reference or for regular gameplay.

| Mod name | Description |
| -------- | ----------- |
| [NoPause](/Mods/NoPause) | Prevents the game from automatically pausing after losing focus (eg. when alt-tabbing or minimizing). |
| [SkipIntro](/Mods/SkipIntro) | Skips the intro sequence and starts the game directly at the main menu. |
| [WakingUpNpcs](/Mods/WakingUpNpcs) | Makes pacified NPCs wake up after a random interval between 4 and 8 minutes. |
| [CodeGen](/Mods/CodeGen) | Generates binding code from the embedded engine typeinfo which can be used by tools like [ResourceParser](https://github.com/OrfeasZ/ZHMTools/tree/master/Tools/ResourceParser). |
| [CertPinBypass](/Mods/CertPinBypass) | Disables SSL certificate pinning, allowing the game to connect to any trusted server instead of only IOI's. |
| [MaxPatchLevel](/Mods/MaxPatchLevel) | Dynamically sets the RPKG patchlevel to 1000, making the game discover patch chunks without having to modify the `packagedefinition.txt` file. |
| [DebugMod](/Mods/DebugMod) | Adds a debug menu to test different SDK functionalities, like 3D rendering and entity information display. |
| [FreeCam](/Mods/FreeCam) | Adds support for an in-game free camera that works with both KB+M and controllers. Can be toggled either from the SDK menu or by pressing `P`. For more details on available controls see [here](/Mods/FreeCam). |
| [DiscordRichPresence](/Mods/DiscordRichPresence) | Sends rich presence updates to Discord with details such as level name, gamemode, etc. **[Requires additional setup!](/Mods/DiscordRichPresence)** |

## Contributing

TODO

## Usage (for developers)

TODO


