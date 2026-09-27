# UnMetal Scanlines & Blur Toggle

Turn UnMetal's scanlines and screen blur on or off independently to remove the retro filter. Windowed and fullscreen settings are combined into a single row, with Left/Right to choose a mode. These options are also available while playing. The Mod also fixes a bug in the original game where switching to a larger window resolution cuts off part of the image.

[日本語](README.ja.md)

## Install

1. Close UnMetal.
2. Extract the release ZIP, or copy this branch's files, into the folder containing `unmetal.exe`.
3. Double-click `install.cmd`, then launch the game with Steam running.

The Mod DLL is included as `UnMetalMod.dll`. No build is required. The installer backs up the original SDL2 DLL automatically. Steam uses its original DLL. Upgrading from this Mod's earlier Steam-based installer restores that Steam DLL automatically.

## Uninstall

Close the game and double-click `uninstall.cmd` to restore the original SDL2 DLL. The Mod preferences in `unmetal_scanlines.ini` are kept for reuse; delete this file as well if you want to reset them.

## Use

Open Video settings from the title screen or the in-game menu. Toggle scanlines and screen blur separately. For the display mode, use Left/Right to select a candidate and confirm to apply it. The checkbox marks the applied mode. Switching between fullscreen and windowed modes may require restarting the game.

The Mod also fixes image clipping when enlarging a window. Effect preferences are saved in `unmetal_scanlines.ini`.

## Compatibility

Supports the Steam version of UnMetal 1.0.13 (build 12471095). The installer checks the game version before making changes.

## Build from source

Source files and the build script are in `src/`. On Windows, install Python, `pefile`, and Visual Studio C++ x86 build tools, then run:

```powershell
python -m pip install pefile
python src/build.py --game-dir "C:\Games\UnMetal"
```

Use an unmodified copy of the supported game. The output is `build/SDL2.dll`. You can select a compiler with `--vcvars` and the path to `vcvarsall.bat`.

## Development

This Mod was created using OpenAI Codex.

## License

The original code and documentation of this Mod are available under the [MIT License](LICENSE). This license does not grant rights to UnMetal or other third-party material. Original game files and SDL2 are not included; the Mod uses the SDL2 DLL from your game installation.
