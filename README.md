# ZHM Mod SDK

A modding SDK and mod loader for HITMAN 3.

![build status](https://github.com/OrfeasZ/ZHMModSDK/workflows/Build/badge.svg)

## Description

This is a community-made modding SDK and mod loader for HITMAN 3. Its purpose is to allow users to easily download and
use mods, and make the creation of more complex runtime mods easier for developers.

Right now it's at a very early stage, with only a few aspects of the engine exposed, but the plan is to further extend
it to support basic entity, AI, and item manipulation.

## Usage (mod loader & mods)

1. Download the latest version of the mod loader and the mods by
   going [here](https://github.com/OrfeasZ/ZHMModSDK/releases/latest), and downloading `ZHMModSDK-Release.zip`. This zip
   file contains the mod loader and a few sample mods.

2. Extract the contents of the `ZHMModSDK-Release.zip` archive to `<drive>:\Path\To\HITMAN3\Retail`, where
   `<drive>:\Path\To\HITMAN3` is the path to your Hitman 3 installation directory. This will be at
   `C:\Program Files\EpicGames\HITMAN3` or `C:\Program Files (x86)\Steam\steamapps\common\HITMAN 3` by default. Make
   sure that you extract the files in the `Retail` folder and not the root `HITMAN3` folder.

3. Run the game like you normally would.

4. When the game opens, you'll see a dialog asking you which mods you'd like to use. Select them and press OK. You might
   need to restart your game for some of them to work.

5. Open the SDK panel with the `~` key (`^` on QWERTZ layouts) to change loaded mods at runtime and to use the menus of
   certain mods.

6. ...

7. Profit?

> NOTE: Some mods might require additional setup. For mods bundled with the SDK, refer to
> the [table below](#sample-mods). For any other mod, make sure to consult its installation instructions.

> NOTE: You can at any time change the mods you're using by pressing the `~` key (`^` on QWERTZ layouts) and selecting
> them, or by editing the `mods.ini` file inside your game's `Retail` folder.

> NOTE: If you are trying to use this on a **Steam Deck** or under **Proton / Linux**, you might need to
> follow [these additional steps](/INSTALL-deck.md).

## Sample mods

There are a few sample mods included in this repository that can be used either as reference or for regular gameplay.

| Mod name                                                       | Description                                                                                                                                                                                                                                                                                                |
|----------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| [AdvancedRating](/Mods/AdvancedRating)                         | Adds an advanced, real-time rating system that tracks player actions through in-game events and AI signals, calculates penalty points, and displays a live performance rating with detailed event history.                                                                                                 |
| [Assets](/Mods/Assets)                                         | Adds an in-game assets menu that allows spawning repository props into the world or inventory, spawning non-repository props by assembly path, and creating actors with configurable outfits and variations.                                                                                               |
| [Clumsy](/Mods/Clumsy)                                         | Makes 47 very clumsy. He just can't seem to figure out how to take cover... **[Requires additional setup!](/Mods/Clumsy)**                                                                                                                                                                                 |
| [DebugCheckKeyEntityEnabler](/Mods/DebugCheckKeyEntityEnabler) | Enables debug key–based entities by patching the engine's debug checks. This includes ZDebugCheckKeyEntity instances used by the game to mute audio buses, which are normally disabled outside debug builds.                                                                                               |
| [DebugMod](/Mods/DebugMod)                                     | Adds in-game debug visualizations for actors, AI behavior, reasoning grids, navigation meshes, planner areas, obstacles, and pathfinding connectivity, with extensive rendering and inspection options.                                                                                                    |
| [DiscordRichPresence](/Mods/DiscordRichPresence)               | Sends rich presence updates to Discord with details such as level name, gamemode, etc. **[Requires additional setup!](/Mods/DiscordRichPresence)**                                                                                                                                                         |
| [Editor](/Mods/Editor)                                         | Displays the entity tree for the current scene, supports real-time property editing, advanced search and filtering, in-scene entity selection, and gizmo visualization. Includes dedicated menus for listing all actors, rooms, and items.                                                                 |
| [FreeCam](/Mods/FreeCam)                                       | Adds support for an in-game free camera with full keyboard/mouse and controller support, as well as an editor-style free camera designed for keyboard/mouse. The free camera can be toggled either from the SDK menu or by pressing `K`. For more details on available controls see [here](/Mods/FreeCam). |
| [Hitmen](/Mods/Hitmen)                                         | Experimental multiplayer foundation that synchronizes player and NPC state between instances using Steam Networking Sockets, enabling multiple Hitman entities within the same scene.                                                                                                                      |
| [MaxPatchLevel](/Mods/MaxPatchLevel)                           | Dynamically sets the RPKG patchlevel to 1000, making the game discover patch chunks without having to modify the `packagedefinition.txt` file.                                                                                                                                                             |
| [Noclip](/Mods/Noclip)                                         | Adds a noclip mode that allows free player movement through the world without collision, controlled via keyboard input and camera orientation.                                                                                                                                                             |
| [NoPause](/Mods/NoPause)                                       | Prevents the game from automatically pausing after losing focus (eg. when alt-tabbing or minimizing).                                                                                                                                                                                                      |
| [OnlineTools](/Mods/OnlineTools)                               | Allows you to change the default domain, along with other online-centric settings. Supersedes CertPinBypass.                                                                                                                                                                                               |
| [Outfits](/Mods/Outfits)                                       | Enables dynamic loading and unloading of outfit bricks from other scenes, including global Season 2 and Season 3 outfits, with automatic chunk and resource package management.                                                                                                                            |
| [Player](/Mods/Player)                                         | Provides player-focused gameplay controls, including invincibility, invisibility, infinite ammo, advanced outfit management, and the ability to teleport all items or actors to the player.                                                                                                                |
| [Randomizer](/Mods/Randomizer)                                 | Adds a configurable item and weapon randomizer that can replace world props, stash items, player inventory items, and NPC inventory items, with support for category filtering, explicit inclusion and exclusion lists.                                                                                    |
| [SkipIntro](/Mods/SkipIntro)                                   | Skips the intro sequence and starts the game directly at the main menu.                                                                                                                                                                                                                                    |
| [TitaniumBullets](/Mods/TitaniumBullets)                       | Enables bullet wall penetration by patching `AmmoConfig` in `pro.repo` (SMF TitaniumBullets behaviour) at runtime.                                                                                                                                                                                         |
| [WakingUpNpcs](/Mods/WakingUpNpcs)                             | Makes pacified NPCs wake up after a random interval between 4 and 8 minutes.                                                                                                                                                                                                                               |
| [World](/Mods/World)                                           | Provides world-level gameplay controls and simulation tweaks. Currently includes a configurable game time multiplier.                                                                                                                                                                                      |

## Uninstalling

To de-activate / uninstall the mod loader, simply delete the extracted `dinput8.dll` from your `HITMAN3\Retail`
directory. You can also
delete all the other files / folders you previously extracted, and the `mods.ini` file (if present).

## Changing the console key

The console key can be changed by editing the `mods.ini` file in your game's `Retail` folder.

To change the console key, open the `mods.ini` file and add the following:

```ini
[sdk]
console_key = 0x29
```

(If the `[sdk]` section already exists, just add the `console_key` key and value)

You can replace `0x29` (tilde / grave key) with a scan code of your choice.
See [this table](https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input#scan-codes) for a list of
scan codes by key (column named Scan 1 Make).

For example, to change it to `F1`, write the following:

```ini
[sdk]
console_key = 0x3B
```

## Changing the UI toggle key

By default, the `F11` key is used to show and hide the SDK UI. You can change this key by editing the `mods.ini` file,
similar to how you change the console key.

To change the UI toggle key, open the `mods.ini` file and add the following:

```ini
[sdk]
ui_toggle_key = 0x57
```

(If the `[sdk]` section already exists, just add the `ui_toggle_key` key and value)

You can replace `0x57` (F11 key) with a scan code of your choice.
See [this table](https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input#scan-codes) for a list of
scan codes by key (column named Scan 1 Make).

For example, to change it to `F2`, write the following:

```ini
[sdk]
ui_toggle_key = 0x3C
```

## Game Logging

**Options**

- `game_state_logging` – Logs when the game changes states
- `scene_loading_logging` – Logs scene loading stages and loading progress percentage.  
- `scaleform_logging` – Logs messages, warnings, and errors sent by the Scaleform UI system.

Logging can be enabled by editing the `mods.ini` file in your game's `Retail` folder.

To enable a logging option, open the `mods.ini` file and add the desired key to the `[sdk]` section.  
For example, to enable Scaleform logging:

```ini
[sdk]
scaleform_logging = true
```

## Usage (for developers)

To find out how to create your own mods or how to extend the SDK, check out
the [wiki](https://github.com/OrfeasZ/ZHMModSDK/wiki). Here are some guides to get you started:

> [**Making mods with the SDK**](https://github.com/OrfeasZ/ZHMModSDK/wiki/Making-mods-with-the-SDK)

> [**Building & debugging the SDK**](https://github.com/OrfeasZ/ZHMModSDK/wiki/Building-&-debugging-the-SDK)

You can also check out the [sample mods](/Mods) for reference.

## FAQ

### I get a message like "The Mod SDK has encountered too many errors and will now unload itself" when I launch the game. What do I do?

This usually happens after a game update, and will be resolved when a new version of the SDK is released. This can take
a few days, so please be patient.

If you're okay with your game breaking in mysterious ways, you can force the SDK to load by adding `force_load = true`
to the `[sdk]` section of your `mods.ini` file (but don't ask for help when it does!)

## Contributing

If you'd like to contribute to the SDK, feel free to open a pull request or an issue. If you're not sure where to start,
check out the [issues](https://github.com/OrfeasZ/ZHMModSDK/issues) tab.
