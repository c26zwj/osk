# Orbit OSK

A compact, touch-friendly on-screen keyboard for Arch Linux running KDE Plasma
on Wayland. The keyboard sends events directly through the kernel's `uinput`
interface, keeping keyboard focus in the target app while still allowing
pointer clicks.

## Build and install on Arch Linux

```sh
sudo pacman -S --needed base-devel cmake qt6-base layer-shell-qt wayland
makepkg -si
```

The package installs the executable and desktop entry. Launch it from the
application menu as **On-Screen Keyboard**.

On Plasma Wayland, the app uses an Xwayland surface when it is available. The
surface stays outside the compositor focus chain, so clicking a key leaves the
target application focused; the keyboard's drag bar moves it directly. Set
`QT_QPA_PLATFORM=wayland` to force the native layer-shell fallback.

Before typing can work, make sure your user can access `/dev/uinput`:

```bash
sudo usermod -aG input "$USER"
```

Log out and back in after changing group membership.

## Controls

- Shift is one-shot; the next printable key uses its shifted value.
- Drag the small dotted bar above the keys to reposition the keyboard.
- Hold printable keys, Space, or Backspace for key repeat.
- Open **Menu** to configure repeat timing, colors, opacity, spacing, and borders.
- Function keys, Backspace, Tab, Caps Lock, Enter, modifiers, Space, and the
  two arrow keys send native key events.
- Restore or quit the keyboard from its system tray icon.
