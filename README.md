
# ZHM Mod SDK

A modding SDK and mod loader for HITMAN 3.

![build status](https://github.com/OrfeasZ/ZHMModSDK/workflows/Build/badge.svg)

## Description

This is a community-made modding SDK and mod loader for HITMAN 3. Its purpose is to allow users to easily download and use mods, and make the creation of more complex runtime mods easier for developers.

Right now it's at a very early stage, with only a few aspects of the engine exposed, but the plan is to further extend it to support basic entity, AI, and item manipulation.

## Usage (mod loader & mods)

1. Download the latest version of the mod loader and the mods by going [here](https://github.com/OrfeasZ/ZHMModSDK/releases/latest), and downloading `mod-loader-RelWithDebInfo.zip` and `mods-RelWithDebInfo.zip`. These zip files contain the mod loader and a few mods respectively.

2. Extract the contents of the `mod-loader-RelWithDebInfo.zip` archive to `<drive>:\Path\To\HITMAN3\Retail`, where `<drive>:\Path\To\HITMAN3` is the path to your Hitman 3 installation directory. This will be at `C:\Program Files\EpicGames\HITMAN3` by default. Make sure that you extract the files in the `Retail` folder and not the root `HITMAN3` folder.

3. Create a new folder called `mods` inside the `Retail` folder you extracted the mod loader to.

4. Extract the `.dll` files for the mods you want to use from the `mods-RelWithDebInfo.zip` archive to the `mods` folder you created in the previous step. You can find a description of what each mod does below.

5. Run the game like you normally would.

6. ...

7. Profit?

## Sample mods

There are a few sample mods included in this repository that can be used either as reference or for regular gameplay.

| Mod name | Description |
| -------- | ----------- |
| [NoPause](/Mods/NoPause) | Prevents the game from automatically pausing after losing focus (eg. when alt-tabbing or minimizing). |
| [SkipIntro](/Mods/SkipIntro) | Skips the intro sequence and starts the game directly at the main menu. |
| [WakingUpNpcs](/Mods/WakingUpNpcs) | Makes pacified NPCs wake up after a random interval between 4 and 8 minutes. |

## Contributing

TODO

## Usage (for developers)

TODO


