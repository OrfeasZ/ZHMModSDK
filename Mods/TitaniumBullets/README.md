# TitaniumBullets

Enables wall-penetrating bullets by patching the in-memory repository (`[assembly:/repository/pro.repo].pc_repo`) at runtime.

This replicates the behaviour of the original Simple Mod Framework TitaniumBullets mod, but as a native ZHMModSDK plugin.

## Usage

- Open the ZHMModSDK menu and toggle **Titanium Bullets**.

## Implementation notes

- Waits for `pro.repo` to be loaded.
- Patches the same repository entries as the SMF mod by swapping `AmmoConfig` to the penetration config (`87ae0524-2f22-4fe0-82e1-84a050b43cf0`).
- Restores original `AmmoConfig` values when disabled.
- Resets internal state on scene unload so the patch can re-apply on mission reloads.

## Credits

- Original SMF TitaniumBullets: https://www.nexusmods.com/hitman3/mods/347
