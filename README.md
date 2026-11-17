<div align="center">

# OSP2-BE

### A Quake III Arena mod for OSP servers and other communities.

A community-driven continuation of the classic arena FPS experience, bringing together game code, virtual machine modules, and assets for players and developers who want to build on OSP-inspired gameplay.

[🌐 Project website](https://osp2.q3a.space/)

<br>

![Quake III Arena](https://img.shields.io/badge/game-Quake%20III%20Arena-222?style=for-the-badge)
![CMake](https://img.shields.io/badge/CMake-3.25%2B-064F8C?style=for-the-badge&logo=cmake&logoColor=white)
![Build system](https://img.shields.io/badge/build-Ninja-2E6E3A?style=for-the-badge)

</div>

---

## What is OSP2-BE?

**OSP2-BE is a mod for Quake III Arena**, made for the fast, competitive style of play associated with OSP servers, while also being useful beyond that setting. It brings together code and ideas from the OSP community and other Quake projects in one buildable codebase.

This repository is for both **players** curious about the project and **developers** who want to compile, inspect, or work on its game code. It contains the mod's source code and build configuration; it is **not a standalone game**. You will need a compatible Quake III Arena installation and client to play.

> **New here?** Visit the [project website](https://osp2.q3a.space/) for more about OSP2-BE. If you want to build from source, start with the steps below.

## At a glance

| | |
| --- | --- |
| **Game** | Quake III Arena |
| **Project** | OSP2-BE mod and source code |
| **Build output** | PK3 asset package, QVMs, and native game modules |
| **Build tools** | CMake 3.25+, Ninja, and a C compiler |

## Build from source

### Requirements

- [CMake](https://cmake.org/) **3.25 or newer**
- [Ninja](https://ninja-build.org/)
- A C compiler: **GCC** or **Clang**, or **MinGW** on Windows

### Compile

From the repository root, configure and build with:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The Release artifacts are placed in `build/Release/osp/`.

### What's in the build?

| File | What it is |
| --- | --- |
| `zz-osp-pak8be.pk3` | Mod asset package and QVMs |
| `vm/cgame.qvm`, `vm/ui.qvm` | Compiled client-game and UI virtual machines |
| `cgamex86_64.so` / `cgamex86_64.dll` | Native client-game module for Linux / Windows |
| `uix86_64.so` / `uix86_64.dll` | Native UI module for Linux / Windows |

### Build options

| CMake option | Default | Description |
| --- | :---: | --- |
| `BUILD_MOD_LIBRARIES` | `ON` | Build native mod libraries |
| `BUILD_MOD_QVMS` | `ON` | Build mod QVMs |
| `BUILD_MOD_PK3` | `ON` | Build the mod PK3 package |
| `BUILD_TESTS` | `OFF` | Build tests |

For example, enable tests when configuring the build:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
```

## Credits

OSP2-BE incorporates code from and work by several projects and contributors. Many thanks to:

- **OSP2 by snems** — [GitHub](https://github.com/snems/OSP2)
- **baseq3a by Cyrax** — [GitHub](https://github.com/ec-/baseq3a)
- **XQ3E Engine** — [GitHub](https://github.com/xq3e/engine)
- **Unlagged Mod by Nail Toronto**
- **Ratmod** — [GitHub](https://github.com/rdntcntrl/ratoa_gamecode)
- **Devotion** — [GitHub](https://github.com/alcachofass/devotion)
- **Quake 3 1.16n extensions (X-Mod)** — [GitHub](https://github.com/navgg/Quake-3-1.16n-extensions)
- **Nemesis Client 1.16n** — [GitHub](https://github.com/dledmonds/quake3-nemesis-1.16n)

Some assets are from [CPMA](https://playmorepromode.com/).

---

<div align="center">

**Built for the arena. Shaped by the community.**

[osp2.q3a.space](https://osp2.q3a.space/)

</div>
