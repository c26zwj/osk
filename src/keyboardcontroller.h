#pragma once

#include <QObject>
#include <QRegion>
#include <QStringList>
#include <QTimer>
#include <QVariantList>

class VirtualKeyboard;
class QQuickWindow;
namespace LayerShellQt { class Window; }

class KeyboardController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool shiftActive READ shiftActive NOTIFY shiftActiveChanged)
    Q_PROPERTY(bool ctrlActive READ ctrlActive NOTIFY ctrlActiveChanged)
    Q_PROPERTY(bool altActive READ altActive NOTIFY altActiveChanged)
    Q_PROPERTY(bool superActive READ superActive NOTIFY superActiveChanged)
    Q_PROPERTY(bool capsLockActive READ capsLockActive NOTIFY capsLockActiveChanged)
    Q_PROPERTY(bool shiftLocked READ shiftLocked NOTIFY shiftActiveChanged)
    Q_PROPERTY(bool ctrlLocked READ ctrlLocked NOTIFY ctrlActiveChanged)
    Q_PROPERTY(bool altLocked READ altLocked NOTIFY altActiveChanged)
    Q_PROPERTY(bool superLocked READ superLocked NOTIFY superActiveChanged)

    Q_PROPERTY(QString backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(int keyRepeatDelay READ keyRepeatDelay WRITE setKeyRepeatDelay NOTIFY keyRepeatDelayChanged)
    Q_PROPERTY(int keyRepeatInterval READ keyRepeatInterval WRITE setKeyRepeatInterval NOTIFY keyRepeatIntervalChanged)
    Q_PROPERTY(bool settingsVisible READ settingsVisible WRITE setSettingsVisible NOTIFY settingsVisibleChanged)
    Q_PROPERTY(bool shortcutPageVisible READ shortcutPageVisible WRITE setShortcutPageVisible NOTIFY shortcutPageVisibleChanged)
    Q_PROPERTY(QVariantList shortcuts READ shortcuts NOTIFY shortcutsChanged)
    Q_PROPERTY(int keyboardWidth READ keyboardWidth WRITE setKeyboardWidth NOTIFY keyboardWidthChanged)
    Q_PROPERTY(int keyboardHeight READ keyboardHeight WRITE setKeyboardHeight NOTIFY keyboardHeightChanged)
    Q_PROPERTY(bool sizePopupVisible READ sizePopupVisible WRITE setSizePopupVisible NOTIFY sizePopupVisibleChanged)
    Q_PROPERTY(bool clipboardPageVisible READ clipboardPageVisible WRITE setClipboardPageVisible NOTIFY clipboardPageVisibleChanged)
    Q_PROPERTY(QStringList clipboardHistory READ clipboardHistory NOTIFY clipboardHistoryChanged)
    Q_PROPERTY(bool keyBorderEnabled READ keyBorderEnabled WRITE setKeyBorderEnabled NOTIFY keyBorderEnabledChanged)
    Q_PROPERTY(int panelX READ panelX WRITE setPanelX NOTIFY panelXChanged)
    Q_PROPERTY(int panelY READ panelY WRITE setPanelY NOTIFY panelYChanged)

public:
    explicit KeyboardController(QObject *parent = nullptr);
    ~KeyboardController() override;

    void setWindow(QQuickWindow *window);
    void setLayerWindow(LayerShellQt::Window *lsw);

    bool shiftActive() const;
    bool ctrlActive() const;
    bool altActive() const;
    bool superActive() const;
    bool capsLockActive() const;
    bool shiftLocked() const;
    bool ctrlLocked() const;
    bool altLocked() const;
    bool superLocked() const;

    QString backgroundColor() const;
    Q_INVOKABLE void setBackgroundColor(const QString &color);

    int keyRepeatDelay() const;
    Q_INVOKABLE void setKeyRepeatDelay(int ms);

    int keyRepeatInterval() const;
    Q_INVOKABLE void setKeyRepeatInterval(int ms);

    bool settingsVisible() const;
    Q_INVOKABLE void setSettingsVisible(bool visible);

    bool shortcutPageVisible() const;
    Q_INVOKABLE void setShortcutPageVisible(bool visible);

    QVariantList shortcuts() const;
    Q_INVOKABLE void addShortcut(const QString &shortcut, const QString &expansion);
    Q_INVOKABLE void editShortcut(int index, const QString &shortcut, const QString &expansion);
    Q_INVOKABLE void removeShortcut(int index);

    Q_INVOKABLE void setShortcutDialogOpen(bool open);
    Q_INVOKABLE void setTextInputMode(bool mode);
    Q_INVOKABLE void pressCtrlCombo(int keyCode);

    int keyboardWidth() const;
    Q_INVOKABLE void setKeyboardWidth(int px);
    int keyboardHeight() const;
    Q_INVOKABLE void setKeyboardHeight(int px);
    bool sizePopupVisible() const;
    Q_INVOKABLE void setSizePopupVisible(bool visible);

    bool clipboardPageVisible() const;
    Q_INVOKABLE void setClipboardPageVisible(bool visible);
    QStringList clipboardHistory() const;
    Q_INVOKABLE void refreshClipboardHistory();
    Q_INVOKABLE void insertClipboardEntry(const QString &text);

    bool keyBorderEnabled() const;
    Q_INVOKABLE void setKeyBorderEnabled(bool enabled);

    int panelX() const;
    Q_INVOKABLE void setPanelX(int x);
    int panelY() const;
    Q_INVOKABLE void setPanelY(int y);
    Q_INVOKABLE void savePanelPosition(int x, int y);

    Q_INVOKABLE void pressKey(int keyCode);
    Q_INVOKABLE void toggleShift();
    Q_INVOKABLE void toggleCtrl();
    Q_INVOKABLE void toggleAlt();
    Q_INVOKABLE void toggleSuper();
    Q_INVOKABLE void toggleCapsLock();

    Q_INVOKABLE bool switchScreen(int direction);

    Q_INVOKABLE void minimizeToTray();
    Q_INVOKABLE void closeApp();

    Q_INVOKABLE void updateInputRegion(int x, int y, int w, int h);

signals:
    void shiftActiveChanged();
    void ctrlActiveChanged();
    void altActiveChanged();
    void superActiveChanged();
    void capsLockActiveChanged();
    void backgroundColorChanged();
    void keyRepeatDelayChanged();
    void keyRepeatIntervalChanged();
    void settingsVisibleChanged();
    void shortcutPageVisibleChanged();
    void shortcutsChanged();
    void keyboardWidthChanged();
    void keyboardHeightChanged();
    void sizePopupVisibleChanged();
    void clipboardPageVisibleChanged();
    void clipboardHistoryChanged();
    void keyBorderEnabledChanged();
    void panelXChanged();
    void panelYChanged();

private:
    void applyModifiers();
    void releaseModifiers();
    void resetOneShot();
    void checkShortcutExpansion();
    void saveShortcuts();
    void saveActiveWindow();
    void restoreActiveWindow();
    static QChar evdevToChar(int keyCode, bool shift);

    VirtualKeyboard *m_vk = nullptr;
    QQuickWindow *m_window = nullptr;
    LayerShellQt::Window *m_layerWindow = nullptr;

    bool m_shift = false;
    bool m_ctrl = false;
    bool m_alt = false;
    bool m_super = false;
    bool m_capsLock = false;
    bool m_shiftLocked = false;
    bool m_ctrlLocked = false;
    bool m_altLocked = false;
    bool m_superLocked = false;

    QString m_backgroundColor = QStringLiteral("#232629");
    int m_keyRepeatDelay = 400;
    int m_keyRepeatInterval = 50;
    bool m_settingsVisible = false;
    bool m_shortcutPageVisible = false;
    int m_keyboardWidth = 900;
    int m_keyboardHeight = 300;
    bool m_sizePopupVisible = false;
    bool m_clipboardPageVisible = false;
    bool m_keyBorderEnabled = false;
    int m_panelX = -1;
    int m_panelY = -1;
    bool m_textInputMode = false;
    QString m_savedWindowId;
    QStringList m_clipboardHistory;
    QVariantList m_shortcuts;

    // Auto-expansion buffer
    QString m_typeBuffer;
    QTimer m_bufferTimer;
};
