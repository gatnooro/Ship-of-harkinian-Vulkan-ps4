# Ship of Harkinian (Vulkan) for PS4

*[Versión en español](README.es.md)*

A PS4 port of [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright) 9.3.1, the PC port of Ocarina of Time, for consoles running GoldHEN. The game code runs natively on the PS4's CPU and is drawn with Vulkan through RADV, Mesa's Radeon driver ported to the PS4 GPU. It is built on the Vulkan renderer of the PS5 port, [damiantw/PS5_Shipwright](https://github.com/damiantw/PS5_Shipwright), and the PS4 runtime of the [SMW](https://github.com/iHaiDeeZ/SMW) and [Wind Waker HD](https://github.com/Nabiscan58/WWHD-Recomp-PS4-Port) ports.

**No game, no ROM and no game data here.** You supply your own copy of the game.

> **Status (01.06):** plays at 60 FPS in most of the game on a PS4 Fat, including with large texture and model packs. Tested on one console only: PS4 Fat CUH-12xx, firmware 13.52, GoldHEN.

It installs as its own title (`SOHV00001`, "Ship of Harkinian (Vulkan)") and keeps its data in `/data/sohvk`, so it can sit next to other Ship of Harkinian ports.

## What you need

- A PS4 with GoldHEN.
- Your own Ocarina of Time data: an `oot.o2r` made from your ROM by Ship of Harkinian 9.3.1 on a PC. A Master Quest `oot-mq.o2r` is optional.
- An FTP client, or another way to copy files to `/data/sohvk` on the console.

## Install

1. Download `SoH-Vulkan-PS4-<version>.pkg` from the Releases page. It is one `.pkg`.
2. Install the `.pkg` with GoldHEN's package installer.
3. Copy your `oot.o2r` to `/data/sohvk/` (create the folder if it is not there).
4. Start "Ship of Harkinian (Vulkan)". The first start prepares the shaders, which takes a moment.

An `oot.o2r` made on a PC gets a one-time notice saying the port did not make it itself. That is expected.

The PS5 port can also convert a ROM (`.z64`, `.n64`, `.v64`) placed in the data folder on the console. That code is still in this port, but it has not been tested on a PS4.

**Updating:** delete the installed version from the home screen, then install the new `.pkg`. Saves, settings, mods and `oot.o2r` in `/data/sohvk` are kept.

## Controls and menu

A click of the touchpad opens and closes the settings menu. The default button layout is the PS5 port's; see [its README](https://github.com/damiantw/PS5_Shipwright#controls). Everything can be remapped under **Settings > Controls**.

## Mods and texture packs

Put `.o2r` or `.otr` files in `/data/sohvk/mods/`. They are enabled on the next start, and their order is set in **Settings > Mod Menu**.

Development was done with Djipi's 3DS texture and model pack (more than 30 files), the heaviest case tested.

## PS4 settings

In the settings menu:

| Setting | Default | What it does |
| --- | --- | --- |
| Keep Game Speed | on | When the CPU cannot draw every frame of the chosen frame rate, fewer frames are drawn (60, 50, 40, 30 or 20 FPS) so the game and its sound keep their normal speed. |
| Skip Off-screen Geometry (experimental) | off | Skips models whose bounding box is off screen, as the N64 does. |
| Anti-aliasing (MSAA) | 1 | 2x is tested. A value of 3 acts as 2, and 5 to 7 as 4. 4x and 8x have not been tested. |

In `/data/sohvk/env.txt` (one `NAME=value` per line):

| Line | Effect |
| --- | --- |
| `SOHVK_MSAA_MAX=2` | Caps MSAA, for a console where a higher value misbehaves. |
| `SOHVK_PS4_PROFILE=1` | Records one minute of CPU samples to `samples.log`, starting 45 seconds after launch. |

## Performance

Measured on a PS4 Fat with version 01.05, which has the same renderer:

- Most scenes: 60 FPS.
- Hyrule Field with Djipi's pack: 55 to 58 FPS on average.
- Scene loads with the pack: 0.6 to 2.7 seconds.
- Memory stayed flat over a 95-minute session.

The limit is one CPU core turning the game's display list into draw calls. The GPU is idle most of the time.

## Known issues

- Short freezes of 0.1 to 0.3 seconds the first time a mod's models and textures come into view.
- In the heaviest views the game can run up to about 4% slow, because Keep Game Speed holds 60 FPS within a tolerance before stepping down.
- The settings menu can stutter slightly while it is open.
- Internal resolution above 100% has not been tested.
- Motion controls are not supported.
- Distant scenery appearing as you approach is the game's own behaviour, not a port bug.

## Logs

Everything is in `/data/sohvk`: the game's log (in `logs/`), `boot-trace.log`, `crash.log` and `samples.log`. The game's log carries a performance report every 10 seconds (frame rate, frame times, draw counts, memory, and what Keep Game Speed decided).

## Building from source

This repository holds patches and port code only. The build takes each project from its own repository:

| Project | Version |
| --- | --- |
| [Shipwright](https://github.com/HarbourMasters/Shipwright) | tag `9.3.1` (`f0599f96`) |
| libultraship (submodule) | `da6daf1a50a5e46d6641408d52227e31e85e46df` |
| Torch (submodule) | `2ab12fe9660aec04e02ee89fe81baed304a1a1d6` |
| [SDL](https://github.com/libsdl-org/SDL) | tag `release-2.32.10` |

Each one gets two layers of patches: `01-ps5-port-*` are PS5_Shipwright's patches, unmodified (commit `a584b3c9`), and `02-ps4-*` are this port's changes on top.

Tools, on Linux: LLVM 21.1.8, the OpenOrbis PS4 Toolchain v0.5.2 (PacBrew), SMW's PS4 SDK `sdk-1` (sysroot and RADV), Vulkan-Headers v1.4.357, CMake and Ninja.

```sh
# 1. Sources and patches
git clone --branch 9.3.1 --recursive https://github.com/HarbourMasters/Shipwright.git src/shipwright
git clone --branch release-2.32.10 https://github.com/libsdl-org/SDL.git src/SDL2
P=$PWD/patches
git -C src/shipwright apply "$P/01-ps5-port-shipwright.patch"
git -C src/shipwright apply "$P/02-ps4-shipwright.patch"
git -C src/shipwright/libultraship apply "$P/01-ps5-port-libultraship.patch"
git -C src/shipwright/libultraship apply "$P/02-ps4-libultraship.patch"
git -C src/shipwright/torch apply "$P/01-ps5-port-torch.patch"
git -C src/shipwright/torch apply "$P/02-ps4-torch.patch"
git -C src/SDL2 apply "$P/01-ps5-port-sdl.patch"
git -C src/SDL2 apply "$P/02-ps4-sdl.patch"

# 2. Tool paths
cp env.example.sh env.sh    # then edit env.sh
source env.sh

# 3. Build: build-soh/soh/soh_eboot/eboot.bin
cmake -G Ninja -S src/shipwright -B build-soh \
  -DCMAKE_TOOLCHAIN_FILE=$PWD/toolchain/ps4.cmake \
  -DCMAKE_PROJECT_Ship_INCLUDE=$PWD/ps4/cmake/ps4-game.cmake \
  -DSDL_DIR=$PWD/src/SDL2 -DSOHVK=$PWD -DCMAKE_BUILD_TYPE=Release
ninja -C build-soh

# 4. Package: out-soh/SoH-Vulkan-PS4.pkg
bash tools/package-soh.sh 01.06
```

The package step needs three files in `pkgfiles/`: `soh.o2r`, `assets.zip` and `ps5_prompts.o2r`. Take them from a release package (`PkgTool.Core pkg_extract`), or make them with PS5_Shipwright's `ps5/tools/assets.sh`.

Layout: `patches/` (the two layers), `port/` (PS4 runtime: heap, threads, crash log, Vulkan on VideoOut, profiler), `ps4/` (platform layer, SDL drivers, CMake glue), `toolchain/`, `tools/` (linking, packaging, profile reader), `vktest/` (a small Vulkan test app).

## Versions

- **01.06**: MSAA values that are not 2, 4 or 8 no longer close the game. Clean-up for the public release.
- **01.05**: pause screen background fixed. Keep Game Speed no longer reacts to one-off loads. The shader compiler is prepared at start-up. Faster resource lookups.
- **01.04**: indexed draw batches and a triangle state cache. Heavy views of Hyrule Field went from 24 ms to about 17 ms of CPU a frame.
- **01.00 to 01.03**: first boot on RADV, sound, Keep Game Speed, first renderer optimizations.

## Credits and licence

This port stands on other people's work: see [CREDITS.md](CREDITS.md). The port's own files are under the MIT licence ([LICENSE](LICENSE)); files that come from other projects keep the licence named in their header.

Not affiliated with Nintendo, Sony or the Ship of Harkinian team.
