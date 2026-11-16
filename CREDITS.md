# Credits and licences

This port is mostly other people's work put together. What is in this repository, where it comes from and under which licence:

| Path | Origin | Licence |
| --- | --- | --- |
| `patches/01-ps5-port-shipwright.patch`, `-libultraship.patch`, `-torch.patch` | [damiantw/PS5_Shipwright](https://github.com/damiantw/PS5_Shipwright), `ps5/patches/` at commit `a584b3c9`, unmodified. They hold the Vulkan renderer and the console support. | MIT, as the projects they change (PS5_Shipwright's `LICENSING.md`) |
| `patches/01-ps5-port-sdl.patch` | The same repository, `ps5/patches/sdl.patch`, unmodified | Zlib, as SDL |
| `patches/02-ps4-*.patch` | This port: PS4 and RADV adaptation, renderer optimizations, resource lookup memo, Keep Game Speed | MIT for the first three, Zlib for the SDL one, as the projects they change |
| `ps4/shaderc/`, `ps4/src/shaderc_glslang.cpp` | This port: the part of shaderc's C++ interface the renderer uses, implemented over the SDK's glslang | MIT (`LICENSE`) |
| `ps4/cmake/`, the rest of `ps4/src/`, `ps4/shader_keys.txt` | Adapted from PS5_Shipwright's `ps5/`, which comes from [griches/PS5_Ghostship](https://github.com/griches/PS5_Ghostship) by Gary Riches. `platform.c` and `platform.h` come from the PS5 Vulkan Template by [Mihawk](https://github.com/mihawk-99). | MIT (each file's header; `licenses/MIT-PS5_Shipwright.txt`) |
| `ps4/sdl/` | SDL2 drivers for the console, from PS5_Shipwright, adapted to VideoOut, scePad and sceAudioOut on the PS4 | Zlib (`licenses/Zlib.txt`) |
| `ps4/third_party/volk/` | [volk](https://github.com/zeux/volk) | MIT (`ps4/third_party/volk/LICENSE.md`) |
| `port/` | The PS4 runtime of [SMW](https://github.com/iHaiDeeZ/SMW) and SSBU by BelmanteGu and ShiroKlein, as carried by the [Wind Waker HD PS4 port](https://github.com/Nabiscan58/WWHD-Recomp-PS4-Port): heap, threads, crash log, Vulkan on VideoOut, profiler | MIT (`licenses/LICENSE-SMW.txt`, `licenses/LICENSE-wwhd-ps4`, `licenses/NOTICE-wwhd-ps4`) |
| `port/third_party/dlmalloc/` | Doug Lea's malloc | Public domain / CC0 |
| `toolchain/`, `tools/` | From the Wind Waker HD PS4 port and SMW, adapted | MIT |
| `vktest/`, `assets/`, the documents | This port | MIT (`LICENSE`) |

## What the build takes from elsewhere

Nothing below is stored here.

| Part | Project | Licence |
| --- | --- | --- |
| The game's code | [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright) by HarbourMasters, built on the [Ocarina of Time decompilation](https://github.com/zeldaret/oot) | see Shipwright's own files |
| The N64 layer and renderer | [libultraship](https://github.com/Kenix3/libultraship) | MIT |
| The ROM's conversion | [Torch](https://github.com/HarbourMasters/Torch) | MIT |
| Windowing, input and sound | [SDL2](https://github.com/libsdl-org/SDL) | Zlib |
| The Vulkan driver | RADV from [Mesa](https://mesa3d.org), ported to the PS4 GPU for [Crema](https://github.com/iHaiDeeZ/Crema) and shipped in SMW's PS4 SDK | MIT |
| The sysroot and libraries | SMW's PS4 SDK `sdk-1` (libc++, glslang, zlib and others) | see the SDK's NOTICE |
| Compiler and packaging tools | LLVM; the OpenOrbis PS4 Toolchain (PacBrew) | Apache-2.0 with LLVM exception; GPL-3.0 |

## In a release package

Besides the program, a package carries `soh.o2r` (Ship of Harkinian's own fonts, shaders and textures), `assets.zip` (the extractor's recipes), `ps5_prompts.o2r` (PlayStation button glyphs built from Kenney's Input Prompts, CC0) and the toolchain's system modules. It holds no game data.

## Thanks

To the Ship of Harkinian team and the decompilation community; to damiantw, Gary Riches and Mihawk for the PS5 work this is built on; to BelmanteGu and ShiroKlein for bringing RADV to the PS4 and for the runtime; to the Wind Waker HD PS4 port; and to Djipi, whose 3DS pack was the test case.

The licence notes above are a reading of each project's terms, not legal advice. If something of yours is credited wrongly, open an issue.
