# OSP2-BE

## [osp2.q3a.space](https://osp2.q3a.space/)

## Build

Requires CMake 3.25+, Ninja, and a C compiler (GCC/Clang, or MinGW on Windows).

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Release output is `build/Release/osp/`:

- `zz-osp-pak8be.pk3` — assets and QVMs
- `vm/cgame.qvm`, `vm/ui.qvm`
- `cgamex86_64.so` / `cgamex86_64.dll` and `uix86_64.so` / `uix86_64.dll`

`BUILD_MOD_LIBRARIES`, `BUILD_MOD_QVMS`, and `BUILD_MOD_PK3` default to `ON`. `BUILD_TESTS` defaults to `OFF`.

## Contributions

OSP2-BE incorporates code from several notable Quake mods and projects:

- **OSP2 by snems**: [GitHub](https://github.com/snems/OSP2)
- **baseq3a by Cyrax**: [GitHub](https://github.com/ec-/baseq3a)
- **XQ3E Engine**: [GitHub](https://github.com/xq3e/engine)
- **Unlagged Mod by Nail Toronto**
- **Ratmod**: [GitHub](https://github.com/rdntcntrl/ratoa_gamecode)
- **Devotion**: [GitHub](https://github.com/alcachofass/devotion)
- **Quake 3 1.16n extensions (X-Mod)**: [GitHub](https://github.com/navgg/Quake-3-1.16n-extensions)
- **Nemesis Client 1.16n**: [GitHub](https://github.com/dledmonds/quake3-nemesis-1.16n)

Some assets taken from:

- **CPMA**: https://playmorepromode.com/
