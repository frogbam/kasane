# Kasane Frontend

A lightweight guitar amp and FX plugin host UI built with Preact.

## Development

```bash
npm install
npm run dev
```

## Build

```bash
npm run build
```

Output is in `dist/` directory as static files ready for JUCE WebView2.

## Architecture

See `../docs/frontend-architecture-spec.md` for the complete specification.

## Tech Stack

- **Framework**: Preact 10
- **State**: @preact/signals
- **Build**: Vite 5
- **Styling**: CSS Variables + vanilla CSS
- **i18n**: Custom JSON-based system (EN, KO, JA, ZH)

## Features

- Audio device selection (input/output)
- Input/output gain controls with VU meters
- Plugin chain management (add, remove, reorder, bypass)
- Plugin editor window integration
- Chromatic tuner
- Theme support (dark/light)
- Multi-language support (EN, KO, JA, ZH)