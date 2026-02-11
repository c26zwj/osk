#include "keyboardcontroller.h"
#include "virtualkeyboard.h"

#include <linux/input-event-codes.h>
#include <QClipboard>
#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QQuickWindow>
#include <QScreen>
#include <QSettings>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusVariant>
#include <LayerShellQt/Window>

// ---------------------------------------------------------------------------
// Evdev keycode → character mapping
// ---------------------------------------------------------------------------
QChar KeyboardController::evdevToChar(int keyCode, bool shift)
{
    // Letters: KEY_Q=16 .. KEY_P=25, KEY_A=30 .. KEY_L=38, KEY_Z=44 .. KEY_M=50
    static const char letterMap[] = {
        // index 0-9: keycodes 16-25 (top row)
        'q','w','e','r','t','y','u','i','o','p',
        // index 10-13: gap (keycodes 26-29)
        0, 0, 0, 0,
        // index 14-22: keycodes 30-38 (home row)
        'a','s','d','f','g','h','j','k','l',
        // index 23-27: gap (keycodes 39-43)
        0, 0, 0, 0, 0,
        // index 28-34: keycodes 44-50 (bottom row)
        'z','x','c','v','b','n','m'
    };

    if (keyCode >= 16 && keyCode <= 50) {
        char c = letterMap[keyCode - 16];
        if (c != 0)
            return shift ? QChar(c).toUpper() : QChar(c);
    }

    // Number row: KEY_1=2 .. KEY_0=11
    static const char numNormal[]  = "1234567890";
    static const char numShift[]   = "!@#$%^&*()";
    if (keyCode >= 2 && keyCode <= 11) {
        int idx = keyCode - 2;
        return QChar(shift ? numShift[idx] : numNormal[idx]);
    }

    // Punctuation keys
    switch (keyCode) {
    case 41: return shift ? QChar('~') : QChar('`');
    case 12: return shift ? QChar('_') : QChar('-');
    case 13: return shift ? QChar('+') : QChar('=');
    case 26: return shift ? QChar('{') : QChar('[');
    case 27: return shift ? QChar('}') : QChar(']');
    case 43: return shift ? QChar('|') : QChar('\\');
    case 39: return shift ? QChar(':') : QChar(';');
    case 40: return shift ? QChar('"') : QChar('\'');
    case 51: return shift ? QChar('<') : QChar(',');
    case 52: return shift ? QChar('>') : QChar('.');
    case 53: return shift ? QChar('?') : QChar('/');
    case 57: return QChar(' ');  // space
    }

    return QChar(); // non-character key
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
KeyboardController::KeyboardController(QObject *parent)
    : QObject(parent)
{
    m_vk = new VirtualKeyboard(this);

    QSettings s;
    m_backgroundColor = s.value(QStringLiteral("backgroundColor"),
                                QStringLiteral("#232629")).toString();
    m_keyRepeatDelay = s.value(QStringLiteral("keyRepeatDelay"), 400).toInt();
    m_keyRepeatInterval = s.value(QStringLiteral("keyRepeatInterval"), 50).toInt();
    m_keyboardWidth = s.value(QStringLiteral("keyboardWidth"), 900).toInt();
    m_keyboardHeight = s.value(QStringLiteral("keyboardHeight"), 300).toInt();
    m_keyBorderEnabled = s.value(QStringLiteral("keyBorderEnabled"), false).toBool();
    m_panelX = s.value(QStringLiteral("panelX"), -1).toInt();
    m_panelY = s.value(QStringLiteral("panelY"), -1).toInt();

    // Load shortcuts
    m_shortcuts = s.value(QStringLiteral("shortcuts")).toList();

    // Migrate old "snippets" data if present
    if (m_shortcuts.isEmpty() && s.contains(QStringLiteral("snippets"))) {
        const QStringList oldSnippets = s.value(QStringLiteral("snippets")).toStringList();
        for (const QString &text : oldSnippets) {
            QVariantMap entry;
            entry[QStringLiteral("shortcut")] = QString();
            entry[QStringLiteral("expansion")] = text;
            m_shortcuts.append(entry);
        }
        saveShortcuts();
        s.remove(QStringLiteral("snippets"));
    }

    // Buffer inactivity timer
    m_bufferTimer.setSingleShot(true);
    m_bufferTimer.setInterval(3000);
    connect(&m_bufferTimer, &QTimer::timeout, this, [this]() {
        m_typeBuffer.clear();
    });
}

KeyboardController::~KeyboardController() = default;

// ---------------------------------------------------------------------------
// Property getters
// ---------------------------------------------------------------------------
bool KeyboardController::shiftActive() const { return m_shift; }
bool KeyboardController::ctrlActive() const { return m_ctrl; }
bool KeyboardController::altActive() const { return m_alt; }
bool KeyboardController::superActive() const { return m_super; }
bool KeyboardController::capsLockActive() const { return m_capsLock; }
bool KeyboardController::shiftLocked() const { return m_shiftLocked; }
bool KeyboardController::ctrlLocked() const { return m_ctrlLocked; }
bool KeyboardController::altLocked() const { return m_altLocked; }
bool KeyboardController::superLocked() const { return m_superLocked; }

QString KeyboardController::backgroundColor() const { return m_backgroundColor; }

void KeyboardController::setBackgroundColor(const QString &color)
{
    if (m_backgroundColor != color) {
        m_backgroundColor = color;
        QSettings().setValue(QStringLiteral("backgroundColor"), color);
        emit backgroundColorChanged();
    }
}

int KeyboardController::keyRepeatDelay() const { return m_keyRepeatDelay; }

void KeyboardController::setKeyRepeatDelay(int ms)
{
    ms = qBound(100, ms, 1000);
    if (m_keyRepeatDelay != ms) {
        m_keyRepeatDelay = ms;
        QSettings().setValue(QStringLiteral("keyRepeatDelay"), ms);
        emit keyRepeatDelayChanged();
    }
}

int KeyboardController::keyRepeatInterval() const { return m_keyRepeatInterval; }

void KeyboardController::setKeyRepeatInterval(int ms)
{
    ms = qBound(20, ms, 200);
    if (m_keyRepeatInterval != ms) {
        m_keyRepeatInterval = ms;
        QSettings().setValue(QStringLiteral("keyRepeatInterval"), ms);
        emit keyRepeatIntervalChanged();
    }
}

bool KeyboardController::settingsVisible() const { return m_settingsVisible; }

void KeyboardController::setSettingsVisible(bool visible)
{
    if (m_settingsVisible != visible) {
        m_settingsVisible = visible;
        emit settingsVisibleChanged();
    }
}

bool KeyboardController::shortcutPageVisible() const { return m_shortcutPageVisible; }

void KeyboardController::setShortcutPageVisible(bool visible)
{
    if (m_shortcutPageVisible != visible) {
        m_shortcutPageVisible = visible;
        emit shortcutPageVisibleChanged();
    }
}

int KeyboardController::keyboardWidth() const { return m_keyboardWidth; }

void KeyboardController::setKeyboardWidth(int px)
{
    px = qBound(450, px, 2000);
    if (m_keyboardWidth != px) {
        m_keyboardWidth = px;
        QSettings().setValue(QStringLiteral("keyboardWidth"), px);
        emit keyboardWidthChanged();
    }
}

int KeyboardController::keyboardHeight() const { return m_keyboardHeight; }

void KeyboardController::setKeyboardHeight(int px)
{
    px = qBound(150, px, 800);
    if (m_keyboardHeight != px) {
        m_keyboardHeight = px;
        QSettings().setValue(QStringLiteral("keyboardHeight"), px);
        emit keyboardHeightChanged();
    }
}

bool KeyboardController::sizePopupVisible() const { return m_sizePopupVisible; }

void KeyboardController::setSizePopupVisible(bool visible)
{
    if (m_sizePopupVisible != visible) {
        m_sizePopupVisible = visible;
        emit sizePopupVisibleChanged();
    }
}

// ---------------------------------------------------------------------------
// Key border
// ---------------------------------------------------------------------------
bool KeyboardController::keyBorderEnabled() const { return m_keyBorderEnabled; }

void KeyboardController::setKeyBorderEnabled(bool enabled)
{
    if (m_keyBorderEnabled != enabled) {
        m_keyBorderEnabled = enabled;
        QSettings().setValue(QStringLiteral("keyBorderEnabled"), enabled);
        emit keyBorderEnabledChanged();
    }
}

// ---------------------------------------------------------------------------
// Panel position persistence
// ---------------------------------------------------------------------------
int KeyboardController::panelX() const { return m_panelX; }

void KeyboardController::setPanelX(int x)
{
    if (m_panelX != x) {
        m_panelX = x;
        emit panelXChanged();
    }
}

int KeyboardController::panelY() const { return m_panelY; }

void KeyboardController::setPanelY(int y)
{
    if (m_panelY != y) {
        m_panelY = y;
        emit panelYChanged();
    }
}

void KeyboardController::savePanelPosition(int x, int y)
{
    m_panelX = x;
    m_panelY = y;
    QSettings s;
    s.setValue(QStringLiteral("panelX"), x);
    s.setValue(QStringLiteral("panelY"), y);
}

// ---------------------------------------------------------------------------
// Clipboard history (KDE Klipper via DBus)
// ---------------------------------------------------------------------------
bool KeyboardController::clipboardPageVisible() const { return m_clipboardPageVisible; }

void KeyboardController::setClipboardPageVisible(bool visible)
{
    if (m_clipboardPageVisible != visible) {
        if (visible)
            saveActiveWindow();
        else
            restoreActiveWindow();
        m_clipboardPageVisible = visible;
        m_textInputMode = visible;
        emit clipboardPageVisibleChanged();
    }
}

QStringList KeyboardController::clipboardHistory() const { return m_clipboardHistory; }

void KeyboardController::refreshClipboardHistory()
{
    QDBusInterface klipper(QStringLiteral("org.kde.klipper"),
                           QStringLiteral("/klipper"),
                           QStringLiteral("org.kde.klipper.klipper"));
    QDBusReply<QStringList> reply = klipper.call(QStringLiteral("getClipboardHistoryMenu"));

    if (reply.isValid()) {
        m_clipboardHistory = reply.value();
    } else {
        m_clipboardHistory.clear();
    }
    emit clipboardHistoryChanged();
}

void KeyboardController::insertClipboardEntry(const QString &text)
{
    if (text.isEmpty()) return;

    QGuiApplication::clipboard()->setText(text);

    // Close the clipboard page (this also restores the previous window)
    setClipboardPageVisible(false);

    // Give the window manager time to activate the previous window before pasting
    QTimer::singleShot(150, this, [this]() {
        if (!m_vk || !m_vk->isReady()) return;
        m_vk->sendKeyPress(KEY_LEFTCTRL);
        m_vk->sendKey(KEY_V);
        m_vk->sendKeyRelease(KEY_LEFTCTRL);
    });
}

// ---------------------------------------------------------------------------
// Shortcuts CRUD
// ---------------------------------------------------------------------------
QVariantList KeyboardController::shortcuts() const { return m_shortcuts; }

void KeyboardController::addShortcut(const QString &shortcut, const QString &expansion)
{
    if (shortcut.isEmpty() || expansion.isEmpty()) return;
    QVariantMap entry;
    entry[QStringLiteral("shortcut")] = shortcut;
    entry[QStringLiteral("expansion")] = expansion;
    m_shortcuts.append(entry);
    saveShortcuts();
    emit shortcutsChanged();
}

void KeyboardController::editShortcut(int index, const QString &shortcut, const QString &expansion)
{
    if (index < 0 || index >= m_shortcuts.size()) return;
    if (shortcut.isEmpty() || expansion.isEmpty()) return;
    QVariantMap entry;
    entry[QStringLiteral("shortcut")] = shortcut;
    entry[QStringLiteral("expansion")] = expansion;
    m_shortcuts[index] = entry;
    saveShortcuts();
    emit shortcutsChanged();
}

void KeyboardController::removeShortcut(int index)
{
    if (index < 0 || index >= m_shortcuts.size()) return;
    m_shortcuts.removeAt(index);
    saveShortcuts();
    emit shortcutsChanged();
}

void KeyboardController::saveShortcuts()
{
    QSettings().setValue(QStringLiteral("shortcuts"), m_shortcuts);
}

// ---------------------------------------------------------------------------
// Window / Layer Shell
// ---------------------------------------------------------------------------
void KeyboardController::setWindow(QQuickWindow *window)
{
    m_window = window;
}

void KeyboardController::setLayerWindow(LayerShellQt::Window *lsw)
{
    m_layerWindow = lsw;
}

void KeyboardController::setShortcutDialogOpen(bool open)
{
    m_textInputMode = open;
    if (!m_layerWindow) return;
    m_layerWindow->setKeyboardInteractivity(
        open ? LayerShellQt::Window::KeyboardInteractivityOnDemand
             : LayerShellQt::Window::KeyboardInteractivityNone);
}

void KeyboardController::setTextInputMode(bool mode)
{
    m_textInputMode = mode;
}

void KeyboardController::pressCtrlCombo(int keyCode)
{
    if (m_textInputMode && m_window) {
        QChar ch = evdevToChar(keyCode, false);
        int qtKey = ch.isNull() ? 0 : ch.toUpper().unicode();
        if (!qtKey) return;
        QKeyEvent press(QEvent::KeyPress, qtKey, Qt::ControlModifier);
        QKeyEvent release(QEvent::KeyRelease, qtKey, Qt::ControlModifier);
        QCoreApplication::sendEvent(m_window, &press);
        QCoreApplication::sendEvent(m_window, &release);
        return;
    }
    if (!m_vk || !m_vk->isReady()) return;
    m_vk->sendKeyPress(KEY_LEFTCTRL);
    m_vk->sendKey(static_cast<uint32_t>(keyCode));
    m_vk->sendKeyRelease(KEY_LEFTCTRL);
}

// ---------------------------------------------------------------------------
// Key dispatch + auto-expansion
// ---------------------------------------------------------------------------
void KeyboardController::pressKey(int keyCode)
{
    // Route to QML text fields when a dialog/filter is focused
    if (m_textInputMode && m_window) {
        Qt::KeyboardModifiers mods = Qt::NoModifier;
        if (m_shift || m_capsLock) mods |= Qt::ShiftModifier;
        if (m_ctrl) mods |= Qt::ControlModifier;
        if (m_alt) mods |= Qt::AltModifier;
        if (m_super) mods |= Qt::MetaModifier;

        int qtKey = 0;
        QString text;
        QChar ch = evdevToChar(keyCode, m_shift || m_capsLock);

        if (!ch.isNull()) {
            qtKey = ch.toUpper().unicode();
            if (!m_ctrl && !m_alt && !m_super)
                text = QString(ch);
        } else {
            switch (keyCode) {
            case KEY_BACKSPACE: qtKey = Qt::Key_Backspace; break;
            case KEY_ENTER:     qtKey = Qt::Key_Return; break;
            case KEY_TAB:       qtKey = Qt::Key_Tab; break;
            case KEY_ESC:       qtKey = Qt::Key_Escape; break;
            case KEY_DELETE:    qtKey = Qt::Key_Delete; break;
            case KEY_LEFT:      qtKey = Qt::Key_Left; break;
            case KEY_RIGHT:     qtKey = Qt::Key_Right; break;
            case KEY_UP:        qtKey = Qt::Key_Up; break;
            case KEY_DOWN:      qtKey = Qt::Key_Down; break;
            case KEY_HOME:      qtKey = Qt::Key_Home; break;
            case KEY_END:       qtKey = Qt::Key_End; break;
            default: return;
            }
        }

        QKeyEvent press(QEvent::KeyPress, qtKey, mods, text);
        QKeyEvent release(QEvent::KeyRelease, qtKey, mods, text);
        QCoreApplication::sendEvent(m_window, &press);
        QCoreApplication::sendEvent(m_window, &release);
        resetOneShot();
        return;
    }

    if (!m_vk || !m_vk->isReady()) return;

    bool isShift = m_shift || m_capsLock;

    // Update type buffer BEFORE sending the key
    if (m_ctrl || m_alt || m_super) {
        // Modifier combo — not regular typing
        m_typeBuffer.clear();
        m_bufferTimer.stop();
    } else if (keyCode == KEY_BACKSPACE) {
        if (!m_typeBuffer.isEmpty())
            m_typeBuffer.chop(1);
    } else if (keyCode == KEY_SPACE || keyCode == KEY_ENTER || keyCode == KEY_TAB || keyCode == KEY_ESC) {
        m_typeBuffer.clear();
        m_bufferTimer.stop();
    } else {
        QChar ch = evdevToChar(keyCode, isShift);
        if (!ch.isNull()) {
            m_typeBuffer.append(ch);
            m_bufferTimer.start();
        }
    }

    // Send the actual key
    applyModifiers();
    m_vk->sendKey(static_cast<uint32_t>(keyCode));
    releaseModifiers();
    resetOneShot();

    // Check for shortcut expansion after the key has been sent
    // (skip when shortcuts page is open — user may be typing into fields)
    if (!m_shortcutPageVisible && !m_ctrl && !m_alt && !m_super)
        checkShortcutExpansion();
}

void KeyboardController::checkShortcutExpansion()
{
    if (m_typeBuffer.isEmpty()) return;

    for (const QVariant &v : std::as_const(m_shortcuts)) {
        const QVariantMap entry = v.toMap();
        const QString trigger = entry.value(QStringLiteral("shortcut")).toString();
        const QString expansion = entry.value(QStringLiteral("expansion")).toString();

        if (trigger.isEmpty() || expansion.isEmpty()) continue;
        if (!m_typeBuffer.endsWith(trigger)) continue;

        // Match found! Backspace to remove the trigger text
        for (int i = 0; i < trigger.length(); ++i) {
            m_vk->sendKey(KEY_BACKSPACE);
        }

        // Clear buffer immediately
        m_typeBuffer.clear();
        m_bufferTimer.stop();

        // Set clipboard, then defer the paste so the Wayland event loop
        // can process the data offer before Ctrl+V is sent
        QGuiApplication::clipboard()->setText(expansion);
        QTimer::singleShot(50, this, [this]() {
            if (!m_vk || !m_vk->isReady()) return;
            m_vk->sendKeyPress(KEY_LEFTCTRL);
            m_vk->sendKey(KEY_V);
            m_vk->sendKeyRelease(KEY_LEFTCTRL);
        });
        break;
    }
}

// ---------------------------------------------------------------------------
// Modifiers
// ---------------------------------------------------------------------------
void KeyboardController::applyModifiers()
{
    if (!m_vk) return;
    if (m_shift || m_capsLock) m_vk->sendKeyPress(KEY_LEFTSHIFT);
    if (m_ctrl)                m_vk->sendKeyPress(KEY_LEFTCTRL);
    if (m_alt)                 m_vk->sendKeyPress(KEY_LEFTALT);
    if (m_super)               m_vk->sendKeyPress(KEY_LEFTMETA);
}

void KeyboardController::releaseModifiers()
{
    if (!m_vk) return;
    if (m_super)               m_vk->sendKeyRelease(KEY_LEFTMETA);
    if (m_alt)                 m_vk->sendKeyRelease(KEY_LEFTALT);
    if (m_ctrl)                m_vk->sendKeyRelease(KEY_LEFTCTRL);
    if (m_shift || m_capsLock) m_vk->sendKeyRelease(KEY_LEFTSHIFT);
}

void KeyboardController::toggleShift()
{
    if (m_shiftLocked) {
        m_shift = false; m_shiftLocked = false;
    } else if (m_shift) {
        m_shiftLocked = true;
    } else {
        m_shift = true;
    }
    emit shiftActiveChanged();
}

void KeyboardController::toggleCtrl()
{
    if (m_ctrlLocked) {
        m_ctrl = false; m_ctrlLocked = false;
    } else if (m_ctrl) {
        m_ctrlLocked = true;
    } else {
        m_ctrl = true;
    }
    emit ctrlActiveChanged();
}

void KeyboardController::toggleAlt()
{
    if (m_altLocked) {
        m_alt = false; m_altLocked = false;
    } else if (m_alt) {
        m_altLocked = true;
    } else {
        m_alt = true;
    }
    emit altActiveChanged();
}

void KeyboardController::toggleSuper()
{
    if (m_superLocked) {
        m_super = false; m_superLocked = false;
    } else if (m_super) {
        m_superLocked = true;
    } else {
        m_super = true;
    }
    emit superActiveChanged();
}

void KeyboardController::toggleCapsLock()
{
    m_capsLock = !m_capsLock;
    emit capsLockActiveChanged();
}

bool KeyboardController::switchScreen(int direction)
{
    if (!m_window) return false;

    auto screens = QGuiApplication::screens();
    if (screens.size() <= 1) return false;

    // Sort screens by x position (left to right)
    std::sort(screens.begin(), screens.end(), [](QScreen *a, QScreen *b) {
        return a->geometry().x() < b->geometry().x();
    });

    QScreen *current = m_window->screen();
    int idx = screens.indexOf(current);
    if (idx < 0) return false;

    int newIdx = idx + direction;
    if (newIdx < 0 || newIdx >= screens.size()) return false;

    // Layer-shell surfaces are bound to a wl_output at creation time.
    // We must hide → change screen → show to force surface recreation
    // on the new output.
    m_window->hide();
    m_window->setScreen(screens[newIdx]);
    m_window->show();
    return true;
}

void KeyboardController::minimizeToTray()
{
    if (m_window)
        m_window->hide();
}

void KeyboardController::closeApp()
{
    QCoreApplication::quit();
}

void KeyboardController::updateInputRegion(int x, int y, int w, int h)
{
    if (!m_window) return;
    m_window->setMask(QRegion(x, y, w, h));
}

// ---------------------------------------------------------------------------
// KWin focus save/restore
// ---------------------------------------------------------------------------
void KeyboardController::saveActiveWindow()
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.kde.KWin"),
        QStringLiteral("/KWin"),
        QStringLiteral("org.freedesktop.DBus.Properties"),
        QStringLiteral("Get"));
    msg << QStringLiteral("org.kde.KWin") << QStringLiteral("activeWindow");

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 100);
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        QVariant outer = reply.arguments().first();
        QDBusVariant dbusVar = outer.value<QDBusVariant>();
        m_savedWindowId = dbusVar.variant().toString();
    } else {
        m_savedWindowId.clear();
    }
}

void KeyboardController::restoreActiveWindow()
{
    if (m_savedWindowId.isEmpty()) return;

    QDBusInterface kwin(QStringLiteral("org.kde.KWin"),
                        QStringLiteral("/KWin"),
                        QStringLiteral("org.kde.KWin"));
    kwin.call(QDBus::NoBlock, QStringLiteral("activateWindow"), m_savedWindowId);
    m_savedWindowId.clear();
}

void KeyboardController::resetOneShot()
{
    if (m_shift && !m_shiftLocked) { m_shift = false; emit shiftActiveChanged(); }
    if (m_ctrl  && !m_ctrlLocked)  { m_ctrl = false;  emit ctrlActiveChanged(); }
    if (m_alt   && !m_altLocked)   { m_alt = false;   emit altActiveChanged(); }
    if (m_super && !m_superLocked) { m_super = false; emit superActiveChanged(); }
}
