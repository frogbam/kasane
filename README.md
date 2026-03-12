# Kasane

> Plug in. Load your tone. Start playing.

<div align="center">
  <br><br><br><br><br><br><br><br>
  <strong>Main app screenshot goes here</strong>
  <br>
  <sub>Suggested image: full app window with plug-in chain, meters, and tuner visible</sub>
  <br><br><br><br><br><br><br><br>
</div>

Kasane is a lightweight Windows app for guitar players who want to hear their sound immediately without opening a full DAW.

Most DAWs are built around recording and production. Kasane is built around one question first:

**how quickly can you get from plugging in your guitar to hearing your tone?**

If your usual flow is:

- turn on the interface
- wait for a DAW to boot
- create or load a project
- set monitoring again
- add amp sim and pedals again

Kasane is built for the shorter path.

Open the app, choose your audio device, load your VST3 amp and effects, and play.

It is a good fit if you want a simple practice rig on your PC instead of a full recording workflow.


## Features

- Standalone guitar practice rig for Windows
- VST3 amp sim and effect hosting
- Fast audio device setup
- Input/output gain control and live meters
- Drag-and-drop plug-in chain
- Native plug-in editor windows
- Built-in tuner
- English, Korean, Japanese, and Simplified Chinese UI


## Current Scope

- Windows
- Standalone app
- VST3 only
- Live guitar monitoring

Kasane is not trying to be a DAW, recorder, or multitrack production tool.

## Building and Installation

### Building from source
What you need:

- Microsoft Edge WebView2 Runtime
- CMake 3.22+
- Visual Studio 2022 or compatible MSVC toolchain
- Node.js 20+ and npm

```powershell
cmake -S . -B build
cmake --build build --config Release
```

After a successful Release build:

```text
build/kasane_artefacts/Release/Kasane.exe
```

## License

Kasane is licensed under `AGPL-3.0-only`.

Kasane uses a number of third-party libraries, SDKs, and runtimes that carry their own licenses and terms:

- The native application and audio host are built on JUCE 8, which is dual-licensed under the AGPLv3 and the commercial JUCE license.
- The VST3 SDK, bundled through JUCE, is licensed under the MIT license by Steinberg Media Technologies GmbH.
- WebView2 is used to host the desktop UI. The Microsoft WebView2 SDK and runtime are subject to Microsoft's license and distribution terms.
- ASIO support is enabled in this project through JUCE. The relevant ASIO code remains subject to the Steinberg ASIO license terms distributed with JUCE.
- The frontend uses Preact, `@preact/signals`, and Vite, which are licensed under the MIT license.
- TypeScript is licensed under the Apache License 2.0.
- User-installed audio plug-ins are not part of Kasane and remain subject to their own licenses.

VST is a trademark of Steinberg Media Technologies GmbH.

See [LICENSE](./LICENSE) for the full license text.
