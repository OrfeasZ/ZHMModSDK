
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

5. Open the SDK panel with the `~` key (`^` on QWERTZ layouts) to change loaded mods at runtime and to use the menus of certain mods.

6. ...

7. Profit?

> NOTE: Some mods might require additional setup. For mods bundled with the SDK, refer to the [table below](#sample-mods). For any other mod, make sure to consult its installation instructions.

> NOTE: You can at any time change the mods you're using by pressing the `~` key (`^` on QWERTZ layouts) and selecting them, or by editing the `mods.ini` file inside your game's `Retail` folder.

> NOTE: If you are trying to use this on a **Steam Deck** or under **Proton / Linux**, you might need to follow [these additional steps](/INSTALL-deck.md).

## Sample mods

There are a few sample mods included in this repository that can be used either as reference or for regular gameplay.

| Mod name | Description |
| -------- | ----------- |
| [NoPause](/Mods/NoPause) | Prevents the game from automatically pausing after losing focus (eg. when alt-tabbing or minimizing). |
| [SkipIntro](/Mods/SkipIntro) | Skips the intro sequence and starts the game directly at the main menu. |
| [WakingUpNpcs](/Mods/WakingUpNpcs) | Makes pacified NPCs wake up after a random interval between 4 and 8 minutes. |
| [OnlineTools](/Mods/OnlineTools) | Allows you to change the default domain, along with other online-centric settings. Supersedes CertPinBypass. |
| [MaxPatchLevel](/Mods/MaxPatchLevel) | Dynamically sets the RPKG patchlevel to 1000, making the game discover patch chunks without having to modify the `packagedefinition.txt` file. |
| [DebugMod](/Mods/DebugMod) | Adds a debug menu to test different SDK functionalities, like 3D rendering and entity information display. |
| [FreeCam](/Mods/FreeCam) | Adds support for an in-game free camera that works with both KB+M and controllers. Can be toggled either from the SDK menu or by pressing `P`. For more details on available controls see [here](/Mods/FreeCam). |
| [DiscordRichPresence](/Mods/DiscordRichPresence) | Sends rich presence updates to Discord with details such as level name, gamemode, etc. **[Requires additional setup!](/Mods/DiscordRichPresence)** |
| [Editor](/Mods/Editor) | A WIP in-engine "editor". Currently supports viewing spawned entities alongside basic lookup and manipulation. |
| [Clumsy](/Mods/Clumsy) | Makes 47 very clumsy. He just can't seem to figure out how to take cover... **[Requires additional setup!](/Mods/Clumsy)** |

## Uninstalling

To de-activate / uninstall the mod loader, simply delete the extracted `dinput8.dll` from your `HITMAN3\Retail` directory. You can also 
delete all the other files / folders you previously extracted, and the `mods.ini` file (if present).

## Changing the console key

The console key can be changed by editing the `mods.ini` file in your game's `Retail` folder.

To change the console key, open the `mods.ini` file and add the following:

```ini
[sdk]
console_key = 0x29
```

(If the `[sdk]` section already exists, just add the `console_key` key and value)

You can replace `0x29` (tilde / grave key) with a scan code of your choice. See [this table](https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input#scan-codes) for a list of scan codes by key (column named Scan 1 Make).

For example, to change it to `F1`, write the following:

```ini
[sdk]
console_key = 0x3B
```

## Changing the UI toggle key

By default, the `F11` key is used to show and hide the SDK UI. You can change this key by editing the `mods.ini` file, similar to how you change the console key.

To change the UI toggle key, open the `mods.ini` file and add the following:

```ini
[sdk]
ui_toggle_key = 0x57
```

(If the `[sdk]` section already exists, just add the `ui_toggle_key` key and value)

You can replace `0x57` (F11 key) with a scan code of your choice. See [this table](https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input#scan-codes) for a list of scan codes by key (column named Scan 1 Make).

For example, to change it to `F2`, write the following:

```ini
[sdk]
ui_toggle_key = 0x3C
```

## Usage (for developers)

To find out how to create your own mods or how to extend the SDK, check out the [wiki](https://github.com/OrfeasZ/ZHMModSDK/wiki). Here are some guides to get you started:

> [**Making mods with the SDK**](https://github.com/OrfeasZ/ZHMModSDK/wiki/Making-mods-with-the-SDK)

> [**Building & debugging the SDK**](https://github.com/OrfeasZ/ZHMModSDK/wiki/Building-&-debugging-the-SDK)

You can also check out the [sample mods](/Mods) for reference.

## Contributing

If you'd like to contribute to the SDK, feel free to open a pull request or an issue. If you're not sure where to start, check out the [issues](https://github.com/OrfeasZ/ZHMModSDK/issues) tab.
