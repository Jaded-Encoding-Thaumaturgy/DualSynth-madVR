### Compiling

```
g++ -shared -std=c++2b -lstdc++ -static -O3 -flto -march=native -D _CRT_SECURE_NO_WARNINGS -D NDEBUG -D _WINDLL -D _WIN64 -D _UNICODE -D UNICODE -I "E:\Cache\Repositories\vapoursynth\include" -o madVR.dll .\src\source.cpp "E:\Cache\Repositories\vapoursynth\msvc_project\x64\Release\VapourSynth.lib" -I "E:\Cache\Repositories\vapoursynth\src\core"
```

Add `-D MADVR_DEBUG` to create a binary that prints frame debug information.

---

### Installing

Put the dll in your plugins folder, then also put these libraries:

#### For 32-bits:

-   madVR.ax
-   madHcNet32.dll
-   mvrSettings32.dll

#### For 64-bits:

-   madVR64.ax
-   madHcNet64.dll
-   mvrSettings64.dll
