#include "keyboardcontroller.h"
#include "virtualkeyboard.h"

#include <linux/input-event-codes.h>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QKeyEvent>
#include <QKeySequence>
#include <QQuickWindow>
#include <QScreen>
#include <QFileDialog>
#include <QProcess>
#include <QSettings>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusVariant>
#include <KGlobalAccel>
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
// Character → evdev keycode (reverse of evdevToChar)
// ---------------------------------------------------------------------------

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
    m_keyPressColor = s.value(QStringLiteral("keyPressColor")).toString();
    m_lockedKeyColor = s.value(QStringLiteral("lockedKeyColor")).toString();
    m_keyBorderColor = s.value(QStringLiteral("keyBorderColor")).toString();
    m_panelX = s.value(QStringLiteral("panelX"), -1).toInt();
    m_panelY = s.value(QStringLiteral("panelY"), -1).toInt();
    m_pagePanelHeight = s.value(QStringLiteral("pagePanelHeight"), 250).toInt();
    m_whisperModelPath = s.value(QStringLiteral("whisperModelPath"),
        QDir::homePath() + QStringLiteral("/.local/share/whisper.cpp/ggml-base.en.bin")).toString();

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

    // New settings
    m_opacity = s.value(QStringLiteral("opacity"), 1.0).toDouble();
    m_fontSize = s.value(QStringLiteral("fontSize"), 14).toInt();
    m_keyRadius = s.value(QStringLiteral("keyRadius"), 6).toInt();
    m_autoHideDelay = s.value(QStringLiteral("autoHideDelay"), 0).toInt();
    m_soundFeedback = s.value(QStringLiteral("soundFeedback"), false).toBool();
    m_closeOnPaste = s.value(QStringLiteral("closeOnPaste"), false).toBool();
    m_closeOnInsertShortcut = s.value(QStringLiteral("closeOnInsertShortcut"), false).toBool();
    m_stickyPosition = s.value(QStringLiteral("stickyPosition"), 0).toInt();
    m_keySpacing = s.value(QStringLiteral("keySpacing"), 3).toInt();
    m_compactMode = s.value(QStringLiteral("compactMode"), false).toBool();
    m_numpadVisible = s.value(QStringLiteral("numpadVisible"), false).toBool();
    m_globalShortcut = s.value(QStringLiteral("globalShortcut"),
        QStringLiteral("Meta+K")).toString();
    m_defaultScreen = s.value(QStringLiteral("defaultScreen"), 0).toInt();
    m_autostartEnabled = QFile::exists(autostartFilePath());

    // Auto-hide timer
    m_autoHideTimer.setSingleShot(true);
    connect(&m_autoHideTimer, &QTimer::timeout, this, [this]() {
        minimizeToTray();
    });

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
        if (visible)
            m_savedWindowIsTerminal = isActiveWindowTerminal();
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
// Custom colors
// ---------------------------------------------------------------------------
QString KeyboardController::keyPressColor() const { return m_keyPressColor; }

void KeyboardController::setKeyPressColor(const QString &color)
{
    if (m_keyPressColor != color) {
        m_keyPressColor = color;
        QSettings().setValue(QStringLiteral("keyPressColor"), color);
        emit keyPressColorChanged();
    }
}

QString KeyboardController::lockedKeyColor() const { return m_lockedKeyColor; }

void KeyboardController::setLockedKeyColor(const QString &color)
{
    if (m_lockedKeyColor != color) {
        m_lockedKeyColor = color;
        QSettings().setValue(QStringLiteral("lockedKeyColor"), color);
        emit lockedKeyColorChanged();
    }
}

QString KeyboardController::keyBorderColor() const { return m_keyBorderColor; }

void KeyboardController::setKeyBorderColor(const QString &color)
{
    if (m_keyBorderColor != color) {
        m_keyBorderColor = color;
        QSettings().setValue(QStringLiteral("keyBorderColor"), color);
        emit keyBorderColorChanged();
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

int KeyboardController::pagePanelHeight() const { return m_pagePanelHeight; }

void KeyboardController::setPagePanelHeight(int px)
{
    px = qBound(150, px, 800);
    if (m_pagePanelHeight != px) {
        m_pagePanelHeight = px;
        QSettings().setValue(QStringLiteral("pagePanelHeight"), px);
        emit pagePanelHeightChanged();
    }
}

QString KeyboardController::whisperModelPath() const { return m_whisperModelPath; }

void KeyboardController::setWhisperModelPath(const QString &path)
{
    if (m_whisperModelPath != path) {
        m_whisperModelPath = path;
        QSettings().setValue(QStringLiteral("whisperModelPath"), path);
        emit whisperModelPathChanged();
    }
}

void KeyboardController::browseWhisperModel()
{
    QString startDir = QFileInfo(m_whisperModelPath).path();
    QString path = QFileDialog::getOpenFileName(nullptr,
        QStringLiteral("Select Whisper Model"), startDir,
        QStringLiteral("Model files (*.bin);;All files (*)"));
    if (!path.isEmpty())
        setWhisperModelPath(path);
}

// ---------------------------------------------------------------------------
// Voice typing
// ---------------------------------------------------------------------------
bool KeyboardController::voiceRecording() const { return m_voiceRecording; }

void KeyboardController::toggleVoiceTyping()
{
    if (m_voiceRecording) {
        // Stop recording
        if (m_recordProcess) {
            m_recordProcess->terminate();
            // startTranscription() will be called when the process finishes
        }
        m_voiceRecording = false;
        emit voiceRecordingChanged();
    } else {
        // Start recording
        m_voiceTempFile = QDir::tempPath() + QStringLiteral("/osk_voice.wav");

        m_recordProcess = new QProcess(this);
        connect(m_recordProcess, &QProcess::finished, this, [this]() {
            m_recordProcess->deleteLater();
            m_recordProcess = nullptr;
            startTranscription();
        });

        m_recordProcess->start(QStringLiteral("pw-record"),
            {QStringLiteral("--rate=16000"),
             QStringLiteral("--channels=1"),
             QStringLiteral("--format=s16"),
             m_voiceTempFile});

        if (!m_recordProcess->waitForStarted(2000)) {
            qWarning("Failed to start pw-record");
            m_recordProcess->deleteLater();
            m_recordProcess = nullptr;
            return;
        }

        m_voiceRecording = true;
        emit voiceRecordingChanged();
    }
}

void KeyboardController::startTranscription()
{
    if (m_voiceTempFile.isEmpty()) return;

    m_transcribeProcess = new QProcess(this);
    connect(m_transcribeProcess, &QProcess::finished, this, [this]() {
        QString output = QString::fromUtf8(m_transcribeProcess->readAllStandardOutput()).trimmed();
        m_transcribeProcess->deleteLater();
        m_transcribeProcess = nullptr;

        // Clean up temp file
        QFile::remove(m_voiceTempFile);
        m_voiceTempFile.clear();

        if (!output.isEmpty()) {
            QDBusInterface klipper(QStringLiteral("org.kde.klipper"),
                                   QStringLiteral("/klipper"),
                                   QStringLiteral("org.kde.klipper.klipper"));
            klipper.call(QStringLiteral("setClipboardContents"), output);
            m_savedWindowIsTerminal = isActiveWindowTerminal();
            QTimer::singleShot(50, this, [this]() { sendPaste(); });
        }
    });

    m_transcribeProcess->start(QStringLiteral("whisper-cli"),
        {QStringLiteral("-m"), m_whisperModelPath,
         QStringLiteral("-nt"),
         QStringLiteral("-np"),
         QStringLiteral("-f"), m_voiceTempFile});

    if (!m_transcribeProcess->waitForStarted(2000)) {
        qWarning("Failed to start whisper-cli");
        m_transcribeProcess->deleteLater();
        m_transcribeProcess = nullptr;
        QFile::remove(m_voiceTempFile);
        m_voiceTempFile.clear();
    }
}


bool KeyboardController::isActiveWindowTerminal()
{
    // Terminal class names that use Ctrl+Shift+V for paste.
    // Matched against the end of the class (e.g. "org.kde.konsole" matches "konsole").
    static const QStringList terminals = {
        QStringLiteral("konsole"),
        QStringLiteral("alacritty"),
        QStringLiteral("kitty"),
        QStringLiteral("foot"),
        QStringLiteral("xterm"),
        QStringLiteral("gnome-terminal-server"),
        QStringLiteral("terminator"),
        QStringLiteral("tilix"),
        QStringLiteral("wezterm"),
        QStringLiteral("st"),
        QStringLiteral("urxvt"),
        QStringLiteral("yakuake"),
    };
    QProcess proc;
    proc.start(QStringLiteral("kdotool"),
               {QStringLiteral("getactivewindow"), QStringLiteral("getwindowclassname")});
    if (proc.waitForFinished(200)) {
        QString resClass = QString::fromUtf8(proc.readAllStandardOutput()).trimmed().toLower();
        for (const QString &term : terminals) {
            if (resClass == term || resClass.endsWith(QLatin1Char('.') + term))
                return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// New settings
// ---------------------------------------------------------------------------
qreal KeyboardController::opacity() const { return m_opacity; }
void KeyboardController::setOpacity(qreal value)
{
    value = qBound(0.1, value, 1.0);
    if (qFuzzyCompare(m_opacity, value)) return;
    m_opacity = value;
    QSettings().setValue(QStringLiteral("opacity"), value);
    emit opacityChanged();
}

int KeyboardController::fontSize() const { return m_fontSize; }
void KeyboardController::setFontSize(int px)
{
    px = qBound(8, px, 28);
    if (m_fontSize == px) return;
    m_fontSize = px;
    QSettings().setValue(QStringLiteral("fontSize"), px);
    emit fontSizeChanged();
}

int KeyboardController::keyRadius() const { return m_keyRadius; }
void KeyboardController::setKeyRadius(int px)
{
    px = qBound(0, px, 20);
    if (m_keyRadius == px) return;
    m_keyRadius = px;
    QSettings().setValue(QStringLiteral("keyRadius"), px);
    emit keyRadiusChanged();
}

int KeyboardController::autoHideDelay() const { return m_autoHideDelay; }
void KeyboardController::setAutoHideDelay(int seconds)
{
    seconds = qBound(0, seconds, 300);
    if (m_autoHideDelay == seconds) return;
    m_autoHideDelay = seconds;
    QSettings().setValue(QStringLiteral("autoHideDelay"), seconds);
    resetAutoHideTimer();
    emit autoHideDelayChanged();
}

bool KeyboardController::soundFeedback() const { return m_soundFeedback; }
void KeyboardController::setSoundFeedback(bool enabled)
{
    if (m_soundFeedback == enabled) return;
    m_soundFeedback = enabled;
    QSettings().setValue(QStringLiteral("soundFeedback"), enabled);
    emit soundFeedbackChanged();
}

bool KeyboardController::closeOnPaste() const { return m_closeOnPaste; }
void KeyboardController::setCloseOnPaste(bool enabled)
{
    if (m_closeOnPaste == enabled) return;
    m_closeOnPaste = enabled;
    QSettings().setValue(QStringLiteral("closeOnPaste"), enabled);
    emit closeOnPasteChanged();
}

bool KeyboardController::closeOnInsertShortcut() const { return m_closeOnInsertShortcut; }
void KeyboardController::setCloseOnInsertShortcut(bool enabled)
{
    if (m_closeOnInsertShortcut == enabled) return;
    m_closeOnInsertShortcut = enabled;
    QSettings().setValue(QStringLiteral("closeOnInsertShortcut"), enabled);
    emit closeOnInsertShortcutChanged();
}

int KeyboardController::stickyPosition() const { return m_stickyPosition; }
void KeyboardController::setStickyPosition(int pos)
{
    pos = qBound(0, pos, 2);
    if (m_stickyPosition == pos) return;
    m_stickyPosition = pos;
    QSettings().setValue(QStringLiteral("stickyPosition"), pos);
    emit stickyPositionChanged();
}

int KeyboardController::keySpacing() const { return m_keySpacing; }
void KeyboardController::setKeySpacing(int px)
{
    px = qBound(0, px, 10);
    if (m_keySpacing == px) return;
    m_keySpacing = px;
    QSettings().setValue(QStringLiteral("keySpacing"), px);
    emit keySpacingChanged();
}

bool KeyboardController::compactMode() const { return m_compactMode; }
void KeyboardController::setCompactMode(bool enabled)
{
    if (m_compactMode == enabled) return;
    m_compactMode = enabled;
    QSettings().setValue(QStringLiteral("compactMode"), enabled);
    emit compactModeChanged();
}

bool KeyboardController::numpadVisible() const { return m_numpadVisible; }
void KeyboardController::setNumpadVisible(bool visible)
{
    if (m_numpadVisible == visible) return;
    m_numpadVisible = visible;
    QSettings().setValue(QStringLiteral("numpadVisible"), visible);
    emit numpadVisibleChanged();
}

bool KeyboardController::autostartEnabled() const { return m_autostartEnabled; }
void KeyboardController::setAutostartEnabled(bool enabled)
{
    if (m_autostartEnabled == enabled) return;

    QString destPath = autostartFilePath();
    if (enabled) {
        QDir().mkpath(QFileInfo(destPath).path());
        QFile f(destPath);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write("[Desktop Entry]\n"
                    "Type=Application\n"
                    "Name=OSK\n"
                    "Exec=" + QCoreApplication::applicationFilePath().toUtf8() + "\n"
                    "Icon=input-keyboard\n"
                    "X-GNOME-Autostart-enabled=true\n");
            f.close();
        }
    } else {
        QFile::remove(destPath);
    }

    m_autostartEnabled = enabled;
    emit autostartEnabledChanged();
}

QString KeyboardController::globalShortcut() const { return m_globalShortcut; }
void KeyboardController::setGlobalShortcut(const QString &shortcut)
{
    if (m_globalShortcut == shortcut) return;
    QKeySequence seq(shortcut);
    if (seq.isEmpty()) return;
    m_globalShortcut = shortcut;
    QSettings().setValue(QStringLiteral("globalShortcut"), shortcut);
    if (m_toggleAction)
        KGlobalAccel::self()->setShortcut(m_toggleAction, {seq});
    emit globalShortcutChanged();
}

int KeyboardController::defaultScreen() const { return m_defaultScreen; }
void KeyboardController::setDefaultScreen(int index)
{
    index = qBound(0, index, qMax(0, QGuiApplication::screens().size() - 1));
    if (m_defaultScreen == index) return;
    m_defaultScreen = index;
    QSettings().setValue(QStringLiteral("defaultScreen"), index);
    emit defaultScreenChanged();
}

void KeyboardController::setToggleAction(QAction *action)
{
    m_toggleAction = action;
}

void KeyboardController::resetAutoHideTimer()
{
    if (m_autoHideDelay > 0) {
        m_autoHideTimer.setInterval(m_autoHideDelay * 1000);
        m_autoHideTimer.start();
    } else {
        m_autoHideTimer.stop();
    }
}

QString KeyboardController::autostartFilePath() const
{
    return QDir::homePath() + QStringLiteral("/.config/autostart/osk.desktop");
}

void KeyboardController::sendPaste()
{
    if (!m_vk || !m_vk->isReady()) return;

    // Terminals need Ctrl+Shift+V instead of Ctrl+V.
    // Use m_savedWindowIsTerminal (captured before opening overlay pages)
    // so we don't query kdotool when the OSK itself may be the active window.
    bool useShift = m_savedWindowIsTerminal;
    m_savedWindowIsTerminal = false;

    if (useShift) m_vk->sendKeyPress(KEY_LEFTSHIFT);
    m_vk->sendKeyPress(KEY_LEFTCTRL);
    m_vk->sendKey(KEY_V);
    m_vk->sendKeyRelease(KEY_LEFTCTRL);
    if (useShift) m_vk->sendKeyRelease(KEY_LEFTSHIFT);
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

    // Tell Klipper to select this entry (moves it to top of its history)
    QDBusInterface klipper(QStringLiteral("org.kde.klipper"),
                           QStringLiteral("/klipper"),
                           QStringLiteral("org.kde.klipper.klipper"));
    klipper.call(QStringLiteral("setClipboardContents"), text);

    // Update local list immediately
    m_clipboardHistory.removeAll(text);
    m_clipboardHistory.prepend(text);
    emit clipboardHistoryChanged();

    // Restore focus to the previous window so paste lands in the right place
    restoreActiveWindow();
    m_textInputMode = false;

    if (m_closeOnPaste) {
        setClipboardPageVisible(false);
    }

    // Paste via Ctrl+V after the window manager restores focus.
    QTimer::singleShot(150, this, [this]() {
        sendPaste();
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

void KeyboardController::insertShortcutExpansion(int index)
{
    if (index < 0 || index >= m_shortcuts.size()) return;
    QVariantMap entry = m_shortcuts.at(index).toMap();
    QString expansion = entry.value(QStringLiteral("expansion")).toString();
    if (expansion.isEmpty()) return;

    if (m_closeOnInsertShortcut) {
        setShortcutPageVisible(false);
    }

    // Set clipboard and paste via Ctrl+V for reliable insertion
    QDBusInterface klipper(QStringLiteral("org.kde.klipper"),
                           QStringLiteral("/klipper"),
                           QStringLiteral("org.kde.klipper.klipper"));
    klipper.call(QStringLiteral("setClipboardContents"), expansion);

    QTimer::singleShot(150, this, [this]() {
        sendPaste();
    });
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
    if (m_window && !m_pendingRegion.isEmpty())
        m_window->setMask(m_pendingRegion);
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
    resetAutoHideTimer();
    if (m_soundFeedback)
        QApplication::beep();

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

        // Set clipboard and paste for reliable insertion
        QDBusInterface klipper(QStringLiteral("org.kde.klipper"),
                               QStringLiteral("/klipper"),
                               QStringLiteral("org.kde.klipper.klipper"));
        klipper.call(QStringLiteral("setClipboardContents"), expansion);

        // Detect terminal now while the target window is still focused
        m_savedWindowIsTerminal = isActiveWindowTerminal();

        QTimer::singleShot(50, this, [this]() {
            sendPaste();
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
    m_pendingRegion = QRegion(x, y, w, h);
    if (!m_window) return;
    m_window->setMask(m_pendingRegion);
}

// ---------------------------------------------------------------------------
// KWin focus save/restore
// ---------------------------------------------------------------------------
void KeyboardController::saveActiveWindow()
{
    // Capture whether the focused window is a terminal before opening overlays,
    // so sendPaste() can use the right key combo later.
    m_savedWindowIsTerminal = isActiveWindowTerminal();

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
