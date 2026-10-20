# Vocal Mid Boost

A 64-bit VST3 / AU effect plugin for dialing in vocal clarity, presence and midrange quality.

## Signal chain

| Stage | Control | Range | Purpose |
|---|---|---|---|
| High-pass | **Low Cut** | 20-300 Hz | Remove rumble and plosive energy |
| Bell @ 300 Hz | **Mud Cut** | 0 to -9 dB | Clear boxiness / mud |
| Bell (sweepable) | **Mid Freq / Mid Boost / Width** | 500 Hz-6 kHz, 0-12 dB, Q 0.3-4 | Main vocal clarity boost |
| High shelf @ 5.5 kHz | **Presence** | 0-8 dB | Forward, intelligible consonants |
| High shelf @ 12 kHz | **Air** | 0-8 dB | Openness and sheen |
| Gain | **Output** | +/-12 dB | Level match |
| | **Bypass** | | Hard bypass |

The editor shows a live combined frequency-response curve. Double-click any knob to reset it.

Starting point for most vocals: Low Cut 80 Hz, Mud Cut -2 dB, Mid Freq 2.8 kHz, Mid Boost +3 dB, Width 1.0.

## Build (64-bit only)

Requires CMake 3.22+ and a C++17 compiler. JUCE 8.0.4 is downloaded automatically.

**Windows (Visual Studio 2022)**
```
cmake -B build -A x64
cmake --build build --config Release
```
VST3 output: `build\VocalMidBoost_artefacts\Release\VST3\Vocal Mid Boost.vst3`
Copy it to `C:\Program Files\Common Files\VST3\`.

**macOS (universal arm64 + x86_64)**
```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
Outputs in `build/VocalMidBoost_artefacts/Release/{VST3,AU}`. Copy to
`~/Library/Audio/Plug-Ins/VST3/` and `~/Library/Audio/Plug-Ins/Components/`.

**Linux (VST3)** needs JUCE's usual dev packages (ALSA, freetype, X11, etc.).

No build machine? Push this folder to GitHub: the included workflow (`.github/workflows/build.yml`)
builds Windows x64 and macOS universal binaries and uploads them as downloadable artifacts.

## Notes

- Change `COMPANY_NAME`, `PLUGIN_MANUFACTURER_CODE` and `BUNDLE_ID` in `CMakeLists.txt` before distributing.
- AU can only be built on macOS. For hosts like Logic, run `auval -v aufx Vmb1 Ycmp` to validate.
- macOS distribution outside your own machine needs code signing and notarization.
- Plugin is mono/stereo, zero latency.
