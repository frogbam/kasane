# Kasane

Kasane is a fast, lightweight guitar amp and FX plugin host for instant playing without a DAW.

It is designed for players who want to plug a guitar into an audio interface, load VST3 amp or effect plugins, and start monitoring immediately without the startup time, recording workflow, and compositional overhead of a full digital audio workstation.

## Why Kasane

Traditional DAWs are built for production, arrangement, editing, and recording. They can do live guitar monitoring, but they are rarely optimized for the simple "plug in and play" path.

Kasane focuses on a narrower job:

- Low-friction audio input and output setup
- Live guitar monitoring through VST3 plugins
- Compact plugin-chain workflow
- Native plugin editor windows
- Built-in tuner
- Multilingual desktop UI

## Features

- Windows standalone app built with JUCE and WebView2
- Preact + TypeScript frontend rendered inside a JUCE WebView2 shell
- VST3 scanning and hosting
- Input and output gain control
- Real-time input and output level meters
- Drag-and-drop plugin chain reordering
- Native plugin editor window support
- Built-in tuner overlay
- English, Korean, Japanese, and Simplified Chinese UI
- Dark and light themes

## Screenshots

Screenshot placeholders can be added here once the UI is finalized.

## Supported Platform and Formats

- Platform: Windows
- Web runtime: Microsoft Edge WebView2 Runtime
- Plugin format: VST3 only

VST2 is intentionally out of scope for this public repository.

## Build

### Requirements

- CMake 3.22+
- Visual Studio 2022 or another compatible MSVC toolchain
- Node.js 20+ and npm
- WebView2 Runtime

### Configure and Build

```powershell
cmake -S . -B build
cmake --build build --config Release
```

The frontend bundle is built automatically with Vite during the CMake build and copied next to the executable.

## Run

After a successful build, the app executable is generated at:

```text
build/KasaneApp_artefacts/Release/Kasane.exe
```

## Architecture

- `JUCE` handles the native window, audio device management, VST3 hosting, and plugin editor windows.
- `AudioProcessorGraph` drives the monitoring chain: input gain -> analysis -> plugin chain -> output gain -> analysis.
- `WebView2` hosts the desktop UI.
- `Preact + TypeScript + Vite` power the frontend.
- A JUCE native/web bridge synchronizes state and commands between the host engine and the UI.

## Repository Layout

- `Source/` native JUCE app, audio host engine, and bridge logic
- `web/` frontend source, localization files, and Vite build config
- `docs/` design notes and UI planning documents

## Roadmap

- Better plugin scan progress and background scanning
- Preset save and recall
- Faster device switching UX
- Session restore for plugin chains
- Expanded plugin state management

## Known Limitations

- Windows only
- VST3 only
- No preset management yet
- No recording or DAW-style editing
- Plugin scan currently follows the default VST3 search locations

## License

Choose and add the project license before public release.
