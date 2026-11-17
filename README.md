# OSP2-BE

**OSP2-BE** is a Quake III Arena project. This repository contains the source code and build configuration for its game modules and asset package.

🌐 **Project website:** [osp2.q3a.space](https://osp2.q3a.space/)

## Build from source

### Requirements

- [CMake](https://cmake.org/) 3.25 or newer
- [Ninja](https://ninja-build.org/)
- A C compiler: GCC or Clang, or MinGW on Windows

### Build

Run these commands from the repository root:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The Release build output is placed in `build/Release/osp/`.

### Build artifacts

| Artifact | Description |
| --- | --- |
| `zz-osp-pak8be.pk3` | Asset package and QVMs |
| `vm/cgame.qvm`, `vm/ui.qvm` | Compiled game and UI virtual machines |
| `cgamex86_64.so` / `cgamex86_64.dll` | Native game module |
| `uix86_64.so` / `uix86_64.dll` | Native UI module |

### CMake options

These options are enabled by default:

| Option | Default | Description |
| --- | --- | --- |
| `BUILD_MOD_LIBRARIES` | `ON` | Build native mod libraries |
| `BUILD_MOD_QVMS` | `ON` | Build mod QVMs |
| `BUILD_MOD_PK3` | `ON` | Build the mod PK3 package |
| `BUILD_TESTS` | `OFF` | Build tests |

Set options during configuration, for example:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
```

## Credits and sources

OSP2-BE incorporates code from or builds on work from these Quake projects and contributors:

- [OSP2 by snems](https://github.com/snems/OSP2)
- [baseq3a by Cyrax](https://github.com/ec-/baseq3a)
- [XQ3E Engine](https://github.com/xq3e/engine)
- Unlagged Mod by Nail Toronto
- [Ratmod](https://github.com/rdntcntrl/ratoa_gamecode)
- [Devotion](https://github.com/alcachofass/devotion)
- [Quake 3 1.16n extensions (X-Mod)](https://github.com/navgg/Quake-3-1.16n-extensions)
- [Nemesis Client 1.16n](https://github.com/dledmonds/quake3-nemesis-1.16n)

Some assets are from [CPMA](https://playmorepromode.com/).
