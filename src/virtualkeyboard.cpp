#include "virtualkeyboard.h"

#include <linux/uinput.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <QDebug>

VirtualKeyboard::VirtualKeyboard(QObject *parent)
    : QObject(parent)
{
    m_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (m_fd < 0) {
        qWarning("Failed to open /dev/uinput — check permissions (user must be in 'input' group)");
        return;
    }

    // Enable key events
    ioctl(m_fd, UI_SET_EVBIT, EV_KEY);

    // Enable all key codes
    for (int i = 0; i < KEY_MAX; i++)
        ioctl(m_fd, UI_SET_KEYBIT, i);

    // Create the virtual input device
    struct uinput_setup setup {};
    strncpy(setup.name, "OSK Virtual Keyboard", UINPUT_MAX_NAME_SIZE - 1);
    setup.id.bustype = BUS_VIRTUAL;
    setup.id.vendor = 0x1234;
    setup.id.product = 0x5678;
    setup.id.version = 1;

    if (ioctl(m_fd, UI_DEV_SETUP, &setup) < 0 ||
        ioctl(m_fd, UI_DEV_CREATE) < 0) {
        qWarning("Failed to create uinput device");
        close(m_fd);
        m_fd = -1;
        return;
    }

    // Small delay for udev to register the device
    usleep(50000);

    m_ready = true;
    qInfo("Virtual keyboard ready (uinput)");
}

VirtualKeyboard::~VirtualKeyboard()
{
    if (m_fd >= 0) {
        ioctl(m_fd, UI_DEV_DESTROY);
        close(m_fd);
    }
}

bool VirtualKeyboard::isReady() const { return m_ready; }

void VirtualKeyboard::emitEvent(int type, int code, int value)
{
    struct input_event ev {};
    ev.type = type;
    ev.code = code;
    ev.value = value;
    write(m_fd, &ev, sizeof(ev));
}

void VirtualKeyboard::sendKey(uint32_t linuxKeyCode)
{
    if (!m_ready) return;
    emitEvent(EV_KEY, linuxKeyCode, 1);
    emitEvent(EV_SYN, SYN_REPORT, 0);
    emitEvent(EV_KEY, linuxKeyCode, 0);
    emitEvent(EV_SYN, SYN_REPORT, 0);
}

void VirtualKeyboard::sendKeyPress(uint32_t linuxKeyCode)
{
    if (!m_ready) return;
    emitEvent(EV_KEY, linuxKeyCode, 1);
    emitEvent(EV_SYN, SYN_REPORT, 0);
}

void VirtualKeyboard::sendKeyRelease(uint32_t linuxKeyCode)
{
    if (!m_ready) return;
    emitEvent(EV_KEY, linuxKeyCode, 0);
    emitEvent(EV_SYN, SYN_REPORT, 0);
}
