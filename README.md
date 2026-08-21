# ShepherdPatch

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)


ShepherdPatch is a modernization and compatibility patch for the PC version of Silent Hill Homecoming.

It focuses on improving how the game behaves on modern systems while preserving the original experience.

![image alt](https://github.com/xowny/ShepherdPatch/blob/main/JZv9u4i.png)

Nexus Mods: https://www.nexusmods.com/silenthillhomecoming/mods/10

## Features

- Borderless windowed support
- DPI-awareness fixes
- Ultrawide display handling
- Correct map, document, and item panels at high resolutions
- 60 FPS support and frame pacing improvements
- Raw mouse input with sensitivity and invert-Y options
- Readable keyboard and mouse prompt labels with live keyboard-rebind updates
- Safer device reset and windowed recovery behavior
- Legacy timer, thread, and input hardening
- Reduced menu movie stutter and playback issues
- Optional startup-logo skip that leaves the game archives unchanged on disk
- Crash dump generation for troubleshooting
- Standalone WinUI configurator with a game-folder selector
- Basic and advanced configuration views
- Persistent Save and Run controls

## Project Layout

- `src/` native patch source
- `tests/` native test suite
- `gui/` WinUI configurator source
- `ShepherdPatch.ini` default configuration
- `CMakeLists.txt` native build entry point

## Notes

- This repository contains source code only.

## Target Game

- Silent Hill Homecoming (PC)
