### Compiling

```
g++ -shared -std=c++2b -lstdc++ -static -O3 -flto -march=athlon64 -D _CRT_SECURE_NO_WARNINGS -D NDEBUG -D _WINDLL -D _WIN64 -D _UNICODE -D UNICODE -I "E:\Cache\Repositories\vapoursynth\include" -o madVR.dll .\src\source.cpp "E:\Cache\Repositories\vapoursynth\msvc_project\x64\Release\VapourSynth.lib" "C:\Program Files (x86)\AviSynth+\FilterSDK\lib\x64\AviSynth.lib" -I "E:\Cache\Repositories\vapoursynth\src\core" -I "E:\Cache\Repositories\AviSynthPlus\avs_core\include"
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


---

### Using

There is only one function:
```core.madVR.Process(clip: VideoNode, commands: string[], adapter: bool)```

`commands` is an array of functions to be ran in the madVR filterchain.


##### Legend
- |x|y|z|...| => can be one of x, y, z
- bool => |on|off|
- int[x..y] => integer in range x to y (inclusive)
- float[x..y] => float in range x to y (inclusive)
- x + |..| => union of a common arg and the list


#### Size functions:

- `upscale(newWidth: int[1..10000], newHeight: int[1..10000], algo: kernels + hq_kernels, antiRing: bool, sigmoidal: bool, superRes: |off|1|2|3|4|, superResLinearLight: bool, superResAntiRing: bool)`

- `upscaleChroma(algo: kernels + hq_kernels + |bilateralOld|bilateralSoft|bilateralSharp|, antiRing: bool, superRes: |off|1|2|3|4|)`

- `downscale(newWidth: int[1..10000], newHeight: int[1..10000], algo: kernels + dw_kernels, antiRing: |strict|relaxed|off|, linearLight: bool, antiBloat: |off|25|50|75|100|125|150|)`

- `crop(left: int[0..10000], top: int[0..10000], width: int[1..10000], height: int[1..10000])`


#### Edges

- `dering(removeDarkHalos: bool)`

- `sharpenEdges(strength: float[0.1..4.0])`

- `crispenEdges(strength: float[0.1..4.0])`

- `enhanceDetail(strength: float[0.1..4.0])`

- `thinEdges(strength: float[0.1..8.0])`

- `lumaSharpen(strength: float[0.01..10.0])`

- `adaptiveSharpen(strength: float[0.1..10.0])`


#### Compression

- `antiBloat(strength: |25|50|75|100|125|150|)`

- `antiRing()`

- `addGrain(strength: int[1..4])`

#### Format

- `setSourceFormat(range: |limited|full|, matrix: |601|709|2020|, primaries: |smpte-c|709|pal|dci|2020|, transfer: |sdr|hdr|, masterDisplayPrimaries: |smpte-c|709|pal|dci|2020||, masterDisplayMaxLuminance: int[120..10000])`

- `setOutputPrimaries(primaries: |smpte-c|709|pal|dci|2020|)`

- `setOutputFormat(format: |rgbTV|rgbPC|yuv420|yuv422|yuv444|, bitdepth: |8|10|12|16|, matrix: |601|709|2020|)`


#### Other

- `convertToSDR(nits: int[120..10000], measureLuminance: bool, highlightRecovery: |off|low|medium|high|veryHigh|extreme|)`

- `rca(strength: int[1..14], quality: |low|medium|high|veryHigh|, chromaStrength: int[1..14])`

- `deband(strength: |low|medium|high|)`



##### Common Args
- kernels => |nearestNeighbor|bilinear|mitchell|catrom|bicubic60|bicubic75|softcubic50|softcubic60|softcubic70|softcubic80|softcubic100|lanczos3|lanczos4|spline3|spline4|jinc|

- up_kernels => |super-xbr25|super-xbr50|super-xbr75|super-xbr100|super-xbr125|super-xbr150|nnedi3-16|nnedi3-32|nnedi3-64|nnedi3-128|nnedi3-256|nguAaLow|nguAaMedium|nguAaHigh|nguAaVeryHigh|nguSoftLow|nguSoftMedium|nguSoftHigh|nguSoftVeryHigh|nguStandardLow|nguStandardMedium|nguStandardHigh|nguStandardVeryHigh|nguSharpLow|nguSharpMedium|nguSharpHigh|nguSharpVeryHigh|

- dw_kernels => 'bicubic100|bicubic125|bicubic150|ssim1d25|ssim1d50|ssim1d75|ssim1d100|ssim2d25|ssim2d50|ssim2d75|ssim2d100'

---

### Example

```
clip.madVR.Process([
    'convertToSDR(4000)',
    'deband(high)',
    'setOutputFormat(yuv444, 16)'
])
```