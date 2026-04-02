# ShepherdPatch

ShepherdPatch is a modernization and compatibility patch for the PC version of Silent Hill Homecoming.

It focuses on improving how the game behaves on modern Windows systems while preserving the original game experience.

Nexus Mods: https://www.nexusmods.com/silenthillhomecoming/mods/10

## Features

- Borderless windowed support
- DPI-awareness fixes
- Ultrawide display handling
- 60 FPS support and frame pacing improvements
- Raw mouse input with sensitivity and invert-Y options
- Safer device reset and windowed recovery behavior
- Legacy timer, thread, and input hardening
- Reduced menu movie stutter for known Bink menu-loop playback issues
- Crash dump generation for troubleshooting
- Standalone WinUI configurator for easier setup

## Project Layout

- `src/` native patch source
- `tests/` native test suite
- `gui/` WinUI configurator source
- `ShepherdPatch.ini` default configuration
- `CMakeLists.txt` native build entry point

## Notes

- This repository contains source code only.
- Build outputs, local scratch data, and packaged binaries are intentionally excluded.

## Target Game

- Silent Hill Homecoming (PC)
