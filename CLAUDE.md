# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

OSK is an on-screen keyboard for KDE Plasma on Wayland. Built with C++20 and Qt 6 (QML frontend, C++ backend). It uses Linux uinput for key injection and KDE's LayerShellQt for Wayland overlay rendering.

## Build Commands

```bash
# Configure (from project root, only needed once or after CMakeLists.txt changes)
cmake -B build

# Build
cmake --build build

# Run
./build/osk
```

No tests or linting are currently configured.

## Dependencies

- CMake 3.25+, C++20 compiler
- Qt 6: Core, Gui, Quick, Qml, Widgets, DBus
- KDE: extra-cmake-modules, LayerShellQt, KF6StatusNotifierItem, KF6GlobalAccel
- Linux uinput kernel module (user must be in `input` group)

## Architecture

Three-layer design:

**QML UI** (`src/qml/`) — All visual components. `main.qml` is the root window with a draggable keyboard panel. `KeyButton.qml` is the reusable key widget with repeat, modifier highlighting, and dual labels. `LettersPage.qml` defines the full QWERTY layout. `Theme.qml` is a singleton providing design tokens and derived colors from the background setting. `SettingsPage.qml` provides color palette, key repeat, and border settings. `ShortcutsPage.qml` is a two-panel CRUD for text expansion shortcuts. `ClipboardPage.qml` browses Klipper clipboard history with filtering.

**KeyboardController** (`src/keyboardcontroller.cpp/h`) — Central C++ backend exposed to QML as context property `"KeyboardController"`. All `Q_INVOKABLE` methods are callable from QML and all `Q_PROPERTY` values bind to QML UI. Manages modifier state (shift/ctrl/alt/super with lock support), settings persistence via QSettings, shortcut text expansion (type-buffer with 3s timeout), clipboard history via D-Bus to Klipper, multi-screen support, and Wayland input region updates.

**VirtualKeyboard** (`src/virtualkeyboard.cpp/h`) — Thin wrapper around Linux `/dev/uinput`. Handles device setup, `sendKey()` (press+release), `sendKeyPress()`, `sendKeyRelease()`, and readiness checks.

**main.cpp** — Initialization sequence: enable LayerShellQt, create QApplication, instantiate KeyboardController, load QML engine, configure layer-shell window as fullscreen overlay, set up KStatusNotifierItem (system tray) and KGlobalAccel (Meta+K toggle).

## Key Conventions

- QML files are embedded via `resources.qrc`, not loaded from filesystem
- All keycodes use Linux evdev constants (e.g., `KEY_A` = 30)
- `KeyboardController::evdevToChar()` maps keycodes to display characters
- Modifier keys can be single-press (apply once then release) or locked (double-click/right-click)
- Settings are stored via QSettings with keys like `backgroundColor`, `keyRepeatDelay`, `panelX`, etc.
- Shortcut expansion works by accumulating typed characters in a buffer, matching against shortcuts, then backspacing the trigger and pasting the expansion via clipboard (deferred Ctrl+V with 50ms delay for Wayland data offer processing)
- Layer-shell keyboard interactivity defaults to `KeyboardInteractivityNone` (pass-through); switches to `OnDemand` when shortcut/clipboard dialogs need text input
- Screen switching uses a hide→setScreen→show sequence to rebind the layer-shell surface
- `protocols/` contains a Wayland virtual keyboard protocol XML (reference only, not compiled)
