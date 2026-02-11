#pragma once

#include <QObject>
#include <cstdint>

// Uses Linux uinput to inject keyboard events at the kernel level.
// Works on any Wayland compositor (KWin, wlroots, etc.)
class VirtualKeyboard : public QObject
{
    Q_OBJECT
public:
    explicit VirtualKeyboard(QObject *parent = nullptr);
    ~VirtualKeyboard() override;

    bool isReady() const;

    void sendKey(uint32_t linuxKeyCode);
    void sendKeyPress(uint32_t linuxKeyCode);
    void sendKeyRelease(uint32_t linuxKeyCode);

private:
    void emitEvent(int type, int code, int value);

    int m_fd = -1;
    bool m_ready = false;
};
