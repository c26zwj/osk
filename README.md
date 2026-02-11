# OSK - On-Screen Keyboard for KDE Plasma Wayland

A lightweight, draggable on-screen keyboard built with Qt 6 and C++20 for KDE Plasma on Wayland. Uses Linux uinput for key injection and KDE LayerShellQt for overlay rendering.

## Features

- Full QWERTY layout with function keys, modifiers, and arrows
- Modifier keys with one-shot and lock modes (Shift, Ctrl, Alt, Meta, Caps Lock)
- Clipboard history integration via KDE Klipper with search/filter
- Text shortcuts that auto-expand trigger words into longer text
- Draggable and resizable with multi-monitor support
- Configurable key repeat, background color, and key borders
- System tray icon and Meta+K global shortcut to toggle visibility
- Wayland layer-shell overlay that passes clicks through to apps below

See [FEATURES.txt](FEATURES.txt) for a detailed feature list.

## Dependencies

- CMake 3.25+
- C++20 compiler (GCC 12+ or Clang 15+)
- Qt 6: Core, Gui, Quick, Qml, Widgets, DBus
- KDE: extra-cmake-modules, LayerShellQt, KF6StatusNotifierItem, KF6GlobalAccel
- Linux uinput kernel module

### Arch Linux

```bash
sudo pacman -S cmake extra-cmake-modules qt6-base qt6-declarative \
    layer-shell-qt kstatusnotifieritem kglobalaccel
```

### User setup

Your user must be in the `input` group to access `/dev/uinput`:

```bash
sudo usermod -aG input $USER
```

Log out and back in for the group change to take effect.

## Building

```bash
cmake -B build
cmake --build build
```

## Running

```bash
./build/osk
```

The keyboard appears as a floating overlay. Drag the top bar to reposition, use the corner handle to resize, or press Meta+K to toggle visibility.

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).
