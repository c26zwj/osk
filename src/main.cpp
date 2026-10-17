#include <QApplication>
#include <QColorDialog>
#include <QCloseEvent>
#include <QCheckBox>
#include <QCursor>
#include <QClipboard>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLockFile>
#include <QMap>
#include <QMouseEvent>
#include <QPointer>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QRegion>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QShowEvent>
#include <QSlider>
#include <QSizeGrip>
#include <QSpinBox>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QToolTip>
#include <QThread>
#include <QMenu>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>
#include <qpa/qplatformnativeinterface.h>
#include <LayerShellQt/Window>
#include <linux/input-event-codes.h>
#include <linux/uinput.h>
#include <fcntl.h>
#include <functional>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <utility>
#include <vector>
#include <sys/ioctl.h>
#include <unistd.h>
#include <wayland-client.h>

class UInputKeyboard final {
public:
    bool initialize() { return ensureDevice(); }

    ~UInputKeyboard() {
        if (fd_ >= 0) {
            if (created_)
                ioctl(fd_, UI_DEV_DESTROY);
            close(fd_);
        }
    }

    bool tap(int keyCode, const std::vector<int> &modifiers = {}) {
        if (!ensureDevice())
            return false;
        for (int modifier : modifiers)
            if (!writeKey(modifier, 1))
                return false;
        if (!writeKey(keyCode, 1) || !sync())
            return false;

        if (!writeKey(keyCode, 0))
            return false;
        for (auto it = modifiers.rbegin(); it != modifiers.rend(); ++it)
            if (!writeKey(*it, 0))
                return false;
        return sync();
    }

    bool typeUnicode(uint codepoint) {
        if (!ensureDevice() || codepoint > 0x10ffff ||
            (codepoint >= 0xd800 && codepoint <= 0xdfff))
            return false;

        if (!writeKey(KEY_LEFTCTRL, 1) || !writeKey(KEY_LEFTSHIFT, 1) ||
            !writeKey(KEY_U, 1) || !sync() || !writeKey(KEY_U, 0) || !sync() ||
            !writeKey(KEY_LEFTSHIFT, 0) || !writeKey(KEY_LEFTCTRL, 0) || !sync())
            return false;

        const QByteArray hex = QByteArray::number(codepoint, 16);
        for (const char digit : hex) {
            int keyCode = 0;
            bool shifted = false;
            if (!asciiKey(static_cast<unsigned char>(digit), keyCode, shifted) ||
                !tap(keyCode, shifted ? std::vector<int>{KEY_LEFTSHIFT} : std::vector<int>{}))
                return false;
        }
        return tap(KEY_ENTER);
    }

    static bool asciiKey(uint character, int &keyCode, bool &shifted) {
        shifted = false;
        static const int letterCodes[26] = {
            KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I,
            KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R,
            KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z
        };
        if (character >= 'a' && character <= 'z') {
            keyCode = letterCodes[character - 'a'];
            return true;
        }
        if (character >= 'A' && character <= 'Z') {
            keyCode = letterCodes[character - 'A'];
            shifted = true;
            return true;
        }
        if (character >= '1' && character <= '9') {
            keyCode = KEY_1 + static_cast<int>(character - '1');
            return true;
        }
        if (character == '0') { keyCode = KEY_0; return true; }

        switch (character) {
        case ' ': keyCode = KEY_SPACE; return true;
        case '\t': keyCode = KEY_TAB; return true;
        case '\n': case '\r': keyCode = KEY_ENTER; return true;
        case '`': keyCode = KEY_GRAVE; return true;
        case '-': keyCode = KEY_MINUS; return true;
        case '=': keyCode = KEY_EQUAL; return true;
        case '[': keyCode = KEY_LEFTBRACE; return true;
        case ']': keyCode = KEY_RIGHTBRACE; return true;
        case '\\': keyCode = KEY_BACKSLASH; return true;
        case ';': keyCode = KEY_SEMICOLON; return true;
        case '\'': keyCode = KEY_APOSTROPHE; return true;
        case ',': keyCode = KEY_COMMA; return true;
        case '.': keyCode = KEY_DOT; return true;
        case '/': keyCode = KEY_SLASH; return true;
        case '~': keyCode = KEY_GRAVE; shifted = true; return true;
        case '!': keyCode = KEY_1; shifted = true; return true;
        case '@': keyCode = KEY_2; shifted = true; return true;
        case '#': keyCode = KEY_3; shifted = true; return true;
        case '$': keyCode = KEY_4; shifted = true; return true;
        case '%': keyCode = KEY_5; shifted = true; return true;
        case '^': keyCode = KEY_6; shifted = true; return true;
        case '&': keyCode = KEY_7; shifted = true; return true;
        case '*': keyCode = KEY_8; shifted = true; return true;
        case '(': keyCode = KEY_9; shifted = true; return true;
        case ')': keyCode = KEY_0; shifted = true; return true;
        case '_': keyCode = KEY_MINUS; shifted = true; return true;
        case '+': keyCode = KEY_EQUAL; shifted = true; return true;
        case '{': keyCode = KEY_LEFTBRACE; shifted = true; return true;
        case '}': keyCode = KEY_RIGHTBRACE; shifted = true; return true;
        case '|': keyCode = KEY_BACKSLASH; shifted = true; return true;
        case ':': keyCode = KEY_SEMICOLON; shifted = true; return true;
        case '"': keyCode = KEY_APOSTROPHE; shifted = true; return true;
        case '<': keyCode = KEY_COMMA; shifted = true; return true;
        case '>': keyCode = KEY_DOT; shifted = true; return true;
        case '?': keyCode = KEY_SLASH; shifted = true; return true;
        default: return false;
        }
    }

private:
    bool ensureDevice() {
        if (fd_ >= 0)
            return true;
        fd_ = open("/dev/uinput", O_WRONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd_ < 0) {
            qWarning("Cannot open /dev/uinput: %s", strerror(errno));
            return false;
        }
        if (ioctl(fd_, UI_SET_EVBIT, EV_KEY) < 0 ||
            ioctl(fd_, UI_SET_EVBIT, EV_SYN) < 0 ||
            ioctl(fd_, UI_SET_EVBIT, EV_REP) < 0) {
            qWarning("Cannot configure /dev/uinput: %s", strerror(errno));
            close(fd_);
            fd_ = -1;
            return false;
        }
        const int keyCodes[] = {
            KEY_ESC, KEY_BACKSPACE, KEY_TAB, KEY_ENTER, KEY_LEFTCTRL,
            KEY_LEFTSHIFT, KEY_RIGHTSHIFT, KEY_LEFTALT, KEY_RIGHTALT,
            KEY_LEFTMETA, KEY_SPACE, KEY_CAPSLOCK, KEY_LEFTBRACE,
            KEY_RIGHTBRACE, KEY_SEMICOLON, KEY_APOSTROPHE, KEY_GRAVE,
            KEY_MINUS, KEY_EQUAL,
            KEY_BACKSLASH, KEY_COMMA, KEY_DOT, KEY_SLASH, KEY_102ND,
            KEY_HOME, KEY_END, KEY_DELETE, KEY_UP, KEY_DOWN, KEY_LEFT,
            KEY_RIGHT, KEY_MENU
        };
        for (int code : keyCodes)
            ioctl(fd_, UI_SET_KEYBIT, code);
        for (int code = KEY_1; code <= KEY_0; ++code)
            ioctl(fd_, UI_SET_KEYBIT, code);
        for (int code = KEY_Q; code <= KEY_P; ++code)
            ioctl(fd_, UI_SET_KEYBIT, code);
        for (int code = KEY_A; code <= KEY_L; ++code)
            ioctl(fd_, UI_SET_KEYBIT, code);
        for (int code = KEY_Z; code <= KEY_M; ++code)
            ioctl(fd_, UI_SET_KEYBIT, code);
        for (int code = KEY_F1; code <= KEY_F10; ++code)
            ioctl(fd_, UI_SET_KEYBIT, code);
        ioctl(fd_, UI_SET_KEYBIT, KEY_F11);
        ioctl(fd_, UI_SET_KEYBIT, KEY_F12);

        uinput_setup setup{};
        setup.id.bustype = BUS_USB;
        setup.id.vendor = 0x1;
        setup.id.product = 0x1;
        setup.id.version = 1;
        std::strncpy(setup.name, "Orbit On-Screen Keyboard", sizeof(setup.name) - 1);
        if (ioctl(fd_, UI_DEV_SETUP, &setup) < 0 ||
            ioctl(fd_, UI_DEV_CREATE) < 0) {
            qWarning("Cannot create the uinput keyboard: %s", strerror(errno));
            close(fd_);
            fd_ = -1;
            return false;
        }
        created_ = true;
        usleep(10000);
        return true;
    }

    bool writeKey(int keyCode, int value) {
        input_event event{};
        event.type = EV_KEY;
        event.code = static_cast<__u16>(keyCode);
        event.value = value;
        return write(fd_, &event, sizeof(event)) == static_cast<ssize_t>(sizeof(event));
    }

    bool sync() {
        input_event event{};
        event.type = EV_SYN;
        event.code = SYN_REPORT;
        return write(fd_, &event, sizeof(event)) == static_cast<ssize_t>(sizeof(event));
    }

    int fd_ = -1;
    bool created_ = false;
};

class DragBar final : public QLabel {
public:
    explicit DragBar(QWidget *window, std::function<void(QPoint, bool)> dragWindow,
                     bool systemMove = false)
        : QLabel("•••", window), dragWindow_(std::move(dragWindow)), systemMove_(systemMove) {
        setObjectName("dragBar");
        setAlignment(Qt::AlignCenter);
        setFixedHeight(14);
        setCursor(Qt::SizeAllCursor);
        setFocusPolicy(Qt::NoFocus);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            if (systemMove_ && window()->windowHandle() &&
                window()->windowHandle()->startSystemMove()) {
                systemMoveStarted_ = true;
                event->accept();
                return;
            }
            dragging_ = true;
            if (QGuiApplication::platformName() != QStringLiteral("wayland")) {
                grabMouse();
                mouseGrabbed_ = true;
            }
            dragWindow_(event->globalPosition().toPoint(), true);
            event->accept();
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (dragging_ && !systemMoveStarted_ && (event->buttons() & Qt::LeftButton)) {
            dragWindow_(event->globalPosition().toPoint(), false);
            event->accept();
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            if (dragging_ && !systemMoveStarted_)
                dragWindow_(event->globalPosition().toPoint(), false);
            dragging_ = false;
            if (systemMoveStarted_)
                systemMoveStarted_ = false;
            else if (mouseGrabbed_)
                releaseMouse();
            mouseGrabbed_ = false;
            event->accept();
        }
    }

private:
    std::function<void(QPoint, bool)> dragWindow_;
    bool systemMove_ = false;
    bool systemMoveStarted_ = false;
    bool dragging_ = false;
    bool mouseGrabbed_ = false;
};

class ResizeGrip final : public QWidget {
public:
    explicit ResizeGrip(QWidget *parent, std::function<void(QPoint)> resizePanel,
                        bool systemResize = false,
                        Qt::Edges edges = Qt::BottomEdge | Qt::RightEdge)
        : QWidget(parent), resizePanel_(std::move(resizePanel)), systemResize_(systemResize), edges_(edges) {
        setFixedSize(18, 18);
        setCursor((edges_ == (Qt::TopEdge | Qt::LeftEdge) ||
                   edges_ == (Qt::BottomEdge | Qt::RightEdge))
                      ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor);
        setToolTip("Drag to resize");
        setStyleSheet("background: transparent; border: none;");
        parent->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (watched == parentWidget() &&
            (event->type() == QEvent::Resize || event->type() == QEvent::Show)) {
            const int s = width();
            const int x = (edges_ & Qt::LeftEdge) ? 0 : parentWidget()->width() - s;
            const int y = (edges_ & Qt::TopEdge) ? 0 : parentWidget()->height() - s;
            move(x, y);
        }
        return QWidget::eventFilter(watched, event);
    }

    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            if (systemResize_ && window()->windowHandle()) {
                window()->windowHandle()->startSystemResize(Qt::BottomEdge | Qt::RightEdge);
                event->accept();
                return;
            }
            start_ = event->globalPosition().toPoint();
            startSize_ = parentWidget()->size();
            startPosition_ = parentWidget()->pos();
            dragging_ = true;
            event->accept();
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (dragging_ && (event->buttons() & Qt::LeftButton)) {
            const QPoint delta = event->globalPosition().toPoint() - start_;
            QSize size = startSize_;
            if (edges_ & Qt::LeftEdge) size.rwidth() -= delta.x();
            if (edges_ & Qt::RightEdge) size.rwidth() += delta.x();
            if (edges_ & Qt::TopEdge) size.rheight() -= delta.y();
            if (edges_ & Qt::BottomEdge) size.rheight() += delta.y();
            QPoint position = startPosition_;
            if (edges_ & Qt::LeftEdge) position.rx() += delta.x();
            if (edges_ & Qt::TopEdge) position.ry() += delta.y();
            resizePanel_(QPoint(size.width(), size.height()));
            if (edges_ & (Qt::LeftEdge | Qt::TopEdge))
                parentWidget()->move(position);
            event->accept();
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            dragging_ = false;
            event->accept();
        }
    }

private:
    std::function<void(QPoint)> resizePanel_;
    bool systemResize_ = false;
    QPoint start_;
    QSize startSize_;
    QPoint startPosition_;
    Qt::Edges edges_;
    bool dragging_ = false;
};

class KeyButton final : public QPushButton {
public:
    explicit KeyButton(const QString &label, QWidget *parent = nullptr)
        : QPushButton(label, parent) {}

    void setRightClickAction(std::function<void()> action) {
        rightClickAction_ = std::move(action);
    }

    void setModifierKey(bool modifier, bool active, bool locked) {
        modifier_ = modifier;
        setProperty("modifier", modifier);
        setProperty("activeModifier", active);
        setProperty("lockedModifier", locked);
        refreshStyle();
    }

    void flashText(const QString &text) {
        const QString original = this->text();
        const int flash = ++flashGeneration_;
        setText(text);
        QTimer::singleShot(180, this, [this, original, flash] {
            if (flash == flashGeneration_)
                setText(original);
        });
    }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (modifier_ && (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)) {
            setProperty("modifierPressed", true);
            refreshStyle();
        }
        QPushButton::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (modifier_) {
            QTimer::singleShot(220, this, [this] {
                setProperty("modifierPressed", false);
                refreshStyle();
            });
        }
        if (event->button() == Qt::RightButton && rect().contains(event->position().toPoint())) {
            if (rightClickAction_)
                rightClickAction_();
            event->accept();
            return;
        }
        QPushButton::mouseReleaseEvent(event);
    }

private:
    void refreshStyle() {
        style()->unpolish(this);
        style()->polish(this);
        update();
    }

    std::function<void()> rightClickAction_;
    bool modifier_ = false;
    int flashGeneration_ = 0;
};

class FocusLineEdit final : public QLineEdit {
public:
    explicit FocusLineEdit(QWidget *parent, std::function<void(QLineEdit *)> clicked,
                           std::function<void()> focusLost = {})
        : QLineEdit(parent), clicked_(std::move(clicked)), focusLost_(std::move(focusLost)) {}

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            if (clicked_)
                clicked_(this);
            window()->activateWindow();
            setFocus(Qt::MouseFocusReason);
        }
        QLineEdit::mousePressEvent(event);
    }

    void focusOutEvent(QFocusEvent *event) override {
        QLineEdit::focusOutEvent(event);
        if (focusLost_)
            focusLost_();
    }

private:
    std::function<void(QLineEdit *)> clicked_;
    std::function<void()> focusLost_;
};

class Keyboard final : public QWidget {
public:
    Keyboard() {
        layerMoveTimer_.setSingleShot(true);
        layerMoveTimer_.setTimerType(Qt::PreciseTimer);
        layerMoveTimer_.setInterval(8);
        connect(&layerMoveTimer_, &QTimer::timeout, this, [this] {
            applyLayerPosition();
        });
        if (!uinput_.initialize())
            qWarning("Keyboard input is unavailable; check access to /dev/uinput");
        setWindowTitle("On-Screen Keyboard");
        setObjectName("keyboard");
        Qt::WindowFlags windowFlags = Qt::Tool | Qt::FramelessWindowHint |
                                      Qt::WindowStaysOnTopHint |
                                      Qt::WindowDoesNotAcceptFocus;
        if (QGuiApplication::platformName() == QStringLiteral("xcb"))
            windowFlags |= Qt::X11BypassWindowManagerHint;
        setWindowFlags(windowFlags);
        setAttribute(Qt::WA_X11DoNotAcceptFocus, true);
        setAttribute(Qt::WA_ShowWithoutActivating, true);
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_TranslucentBackground, false);
        settings_.beginGroup("appearance");
        repeatDelay_ = settings_.value("repeatDelay", 450).toInt();
        repeatInterval_ = settings_.value("repeatInterval", 55).toInt();
        keySpacing_ = settings_.value("keySpacing", 2).toInt();
        borderWidth_ = settings_.value("borderWidth", 1).toInt();
        opacity_ = settings_.value("opacity", 100).toInt();
        keyColor_ = settings_.value("keyColor", "#202c3d").toString();
        borderColor_ = settings_.value("borderColor", "#101722").toString();
        keyLabelColor_ = settings_.value("keyLabelColor", "#eef2f7").toString();
        hoverColor_ = settings_.value("hoverColor", "#3a506d").toString();
        settings_.endGroup();

        panel_ = new QWidget(this);
        panel_->setObjectName("keyboardPanel");
        panel_->setAttribute(Qt::WA_StyledBackground, true);
        panel_->setMinimumSize(420, 150);
        panel_->resize(580 + keySpacing_, 199);
        settings_.beginGroup("window");
        int savedWidth = settings_.value("width", panel_->width()).toInt();
        int savedHeight = settings_.value("height", panel_->height()).toInt();
        savedPanelPosition_ = QPoint(settings_.value("x", 0).toInt(),
                                     settings_.value("y", 0).toInt());
        restorePanelPosition_ = settings_.contains("x") && settings_.contains("y");
        settings_.endGroup();
        const QRect availableGeometry = QGuiApplication::primaryScreen()
                                            ? QGuiApplication::primaryScreen()->availableGeometry()
                                            : QRect();
        if (!availableGeometry.isEmpty() &&
            (savedWidth >= availableGeometry.width() ||
             savedHeight >= availableGeometry.height())) {
            savedWidth = panel_->width();
            savedHeight = panel_->height();
            savedPanelPosition_ = {};
            restorePanelPosition_ = false;
            settings_.beginGroup("window");
            settings_.remove("width");
            settings_.remove("height");
            settings_.remove("x");
            settings_.remove("y");
            settings_.endGroup();
        }
        panel_->resize(qMax(panel_->minimumWidth(), savedWidth),
                       qMax(panel_->minimumHeight(), savedHeight));
        opacityEffect_ = new QGraphicsOpacityEffect(panel_);
        panel_->setGraphicsEffect(opacityEffect_);

        auto *panelLayout = new QVBoxLayout(panel_);
        panelLayout->setContentsMargins(0, 0, 0, 0);
        panelLayout->setSpacing(1);

        auto *topBar = new QHBoxLayout;
        topBar->setContentsMargins(0, 0, 0, 0);
        topBar->setSpacing(1);
        topBar->addWidget(new DragBar(panel_, [this](QPoint pointer, bool begin) {
            dragTo(pointer, begin);
        }), 1);
        auto *monitorButton = new QPushButton("⇄", panel_);
        monitorButton->setObjectName("monitorButton");
        monitorButton->setFocusPolicy(Qt::NoFocus);
        monitorButton->setFixedSize(22, 14);
        monitorButton->setToolTip("Move keyboard to next monitor");
        connect(monitorButton, &QPushButton::clicked, this, [this] {
            const auto screens = QGuiApplication::screens();
            if (screens.size() < 2)
                return;
            QScreen *current = QGuiApplication::screenAt(frameGeometry().center());
            if (!current)
                current = QGuiApplication::primaryScreen();
            int index = screens.indexOf(current);
            QScreen *next = screens.at((index + 1) % screens.size());
            const QRect source = current->availableGeometry();
            const QRect destination = next->availableGeometry();
            const QPoint offset = frameGeometry().topLeft() - source.topLeft();
            const int x = qBound(destination.left(), destination.left() + offset.x(),
                                 destination.right() - width() + 1);
            const int y = qBound(destination.top(), destination.top() + offset.y(),
                                 destination.bottom() - height() + 1);
            movePanel(QPoint(x, y));
            raise();
        });
        topBar->addWidget(monitorButton);
        auto *minimizeButton = new QPushButton("−", panel_);
        minimizeButton->setObjectName("minimizeButton");
        minimizeButton->setFocusPolicy(Qt::NoFocus);
        minimizeButton->setFixedSize(22, 14);
        minimizeButton->setToolTip("Minimize keyboard");
        connect(minimizeButton, &QPushButton::clicked, this, &Keyboard::minimizeToTray);
        topBar->addWidget(minimizeButton);
        auto *closeButton = new QPushButton("×", panel_);
        closeButton->setObjectName("closeButton");
        closeButton->setFocusPolicy(Qt::NoFocus);
        closeButton->setFixedSize(22, 14);
        closeButton->setToolTip("Close keyboard");
        connect(closeButton, &QPushButton::clicked, qApp, &QApplication::quit);
        topBar->addWidget(closeButton);
        auto *topBarSpacer = new QWidget(panel_);
        topBarSpacer->setObjectName("topBarSpacer");
        topBarSpacer->setFixedWidth(20);
        topBar->addWidget(topBarSpacer);
        panelLayout->addLayout(topBar);

        tray_ = new QSystemTrayIcon(QIcon::fromTheme("input-keyboard"), this);
        auto *trayMenu = new QMenu;
        auto *restoreAction = trayMenu->addAction("Restore keyboard");
        trayMenu->addSeparator();
        auto *quitAction = trayMenu->addAction("Quit");
        tray_->setContextMenu(trayMenu);
        connect(restoreAction, &QAction::triggered, this, [this] { restoreFromTray(); });
        connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
        connect(tray_, &QSystemTrayIcon::activated, this,
                [this](QSystemTrayIcon::ActivationReason reason) {
                    if (reason == QSystemTrayIcon::Trigger ||
                        reason == QSystemTrayIcon::DoubleClick)
                        restoreFromTray();
                });
        tray_->show();

        body_ = new QHBoxLayout;
        body_->setContentsMargins(0, 0, 0, 0);
        body_->setSpacing(keySpacing_);

        auto *mainKeys = new QWidget(panel_);
        mainKeys->setMinimumWidth(0);
        keys_ = new QVBoxLayout(mainKeys);
        keys_->setContentsMargins(2, 0, 0, 2);
        keys_->setSpacing(2);
        body_->addWidget(mainKeys);

        auto *sidePanel = new QWidget(panel_);
        sidePanel->setFixedWidth(48);
        sideKeys_ = new QVBoxLayout(sidePanel);
        sideKeys_->setContentsMargins(0, 0, 0, 2);
        sideKeys_->setSpacing(keySpacing_);
        auto addSideKey = [this](const QString &label, const QString &action,
                                 const QString &tooltip) {
            auto *button = key(label, action);
            if (action == "CUT" || action == "COPY" || action == "PASTE")
                button->setProperty("clipboardAction", action);
            button->setToolTip(tooltip);
            sideKeys_->addWidget(button, 1);
        };
        addSideKey("Del", "DELETE", "Delete");
        addSideKey("✂", "CUT", "Cut (Ctrl+X)");
        addSideKey("⧉", "COPY", "Copy (Ctrl+C)");
        addSideKey("▤", "PASTE", "Paste (Ctrl+V)");
        addSideKey("|←", "HOME", "Home");
        addSideKey("→|", "END", "End");
        body_->addWidget(sidePanel);
        panelLayout->addLayout(body_);

        const auto resizePanel = [this](QPoint requestedSize) {
            resize(qMax(minimumWidth(), requestedSize.x()),
                   qMax(minimumHeight(), requestedSize.y()));
            panel_->resize(size());
            panel_->layout()->activate();
        };
        resizeGrips_.append(new ResizeGrip(this, resizePanel, false,
                                           Qt::TopEdge | Qt::LeftEdge));
        resizeGrips_.append(new ResizeGrip(this, resizePanel, false,
                                           Qt::TopEdge | Qt::RightEdge));
        resizeGrips_.append(new ResizeGrip(this, resizePanel, false,
                                           Qt::BottomEdge | Qt::LeftEdge));
        resizeGrips_.append(new ResizeGrip(this, resizePanel, false,
                                           Qt::BottomEdge | Qt::RightEdge));

        buildKeys();
        applyStyle();

        resize(panel_->size());
        panel_->move(0, 0);
        setWindowState(Qt::WindowNoState);
        QScreen *placementScreen = nullptr;
        if (restorePanelPosition_) {
            for (QScreen *screen : QGuiApplication::screens()) {
                if (screen->availableGeometry().contains(savedPanelPosition_)) {
                    placementScreen = screen;
                    break;
                }
            }
        }
        if (!placementScreen)
            placementScreen = QGuiApplication::primaryScreen();
        if (placementScreen) {
            const QRect area = placementScreen->availableGeometry();
            const QPoint desired = restorePanelPosition_
                                       ? savedPanelPosition_
                                       : area.center() - rect().center();
            panelPosition_ = QPoint(
                qBound(area.left(), desired.x(),
                       qMax(area.left(), area.right() - width() + 1)),
                qBound(area.top(), desired.y(),
                       qMax(area.top(), area.bottom() - height() + 1)));
        }

        if (QGuiApplication::platformName() == QStringLiteral("xcb")) {
            // X11BypassWindowManagerHint keeps the keyboard outside the
            // compositor's focus chain. Keyboard input therefore goes to the
            // application that was focused before a key was clicked.
            winId();
        }

        if (QGuiApplication::platformName() == QStringLiteral("wayland")) {
            // LayerShellQt gives Plasma an explicit overlay surface. Anchor
            // only the top and left edges so the surface remains the size of
            // the keyboard instead of becoming an output-sized layer.
            winId();
            layer_ = LayerShellQt::Window::get(windowHandle());
            if (layer_) {
                layer_->setLayer(LayerShellQt::Window::LayerOverlay);
                layer_->setKeyboardInteractivity(
                    LayerShellQt::Window::KeyboardInteractivityNone);
                layer_->setActivateOnShow(false);
                layer_->setCloseOnDismissed(false);
                layer_->setScope(QStringLiteral("orbit-osk"));
                layer_->setExclusiveZone(0);
                LayerShellQt::Window::Anchors anchors;
                anchors.setFlag(LayerShellQt::Window::AnchorTop);
                anchors.setFlag(LayerShellQt::Window::AnchorLeft);
                layer_->setAnchors(anchors);
                if (placementScreen)
                    layer_->setScreen(placementScreen);
                layer_->setDesiredSize(size());
                const QPoint localPosition = placementScreen
                    ? panelPosition_ - placementScreen->geometry().topLeft()
                    : panelPosition_;
                layer_->setMargins(QMargins(localPosition.x(), localPosition.y(), 0, 0));
            }
        }

        if (!layer_) {
            showNormal();
            resize(panel_->size());
            move(panelPosition_);
        } else {
            show();
        }
        QTimer::singleShot(0, this, [this] {
            showInsertDialog(true);
            if (layer_)
                show();
            else
                showNormal();
            raise();
        });
    }

protected:
    void showEvent(QShowEvent *event) override {
        QWidget::showEvent(event);
    }

    void closeEvent(QCloseEvent *event) override {
        layerMoveTimer_.stop();
        settings_.beginGroup("window");
        settings_.setValue("width", panel_->width());
        settings_.setValue("height", panel_->height());
        settings_.setValue("x", panelPosition_.x());
        settings_.setValue("y", panelPosition_.y());
        settings_.endGroup();
        settings_.sync();
        QWidget::closeEvent(event);
    }

    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        if (panel_)
            panel_->resize(size());
        if (layer_)
            layer_->setDesiredSize(size());
        const int gripSize = resizeGrips_.isEmpty() ? 0 : resizeGrips_.first()->width();
        if (resizeGrips_.size() == 4) {
            resizeGrips_[0]->move(0, 0);
            resizeGrips_[1]->move(width() - gripSize, 0);
            resizeGrips_[2]->move(0, height() - gripSize);
            resizeGrips_[3]->move(width() - gripSize, height() - gripSize);
        }
    }

private:
    enum class ModifierState { Off, Armed, Locked };

    QVBoxLayout *keys_{};
    QVBoxLayout *sideKeys_{};
    QHBoxLayout *body_{};
    QWidget *panel_{};
    QList<ResizeGrip *> resizeGrips_;
    QPointer<QLineEdit> activeEditor_;
    QDialog insertDialog_;
    QPointer<QWindow> insertTargetWindow_;
    bool insertDialogInitialized_ = false;
    QPoint insertDragPointer_;
    QPoint insertDragPosition_;
    QStringList snippetValues_;
    QGraphicsOpacityEffect *opacityEffect_{};
    QSystemTrayIcon *tray_{};
    QSettings settings_;
    int repeatDelay_ = 450;
    int repeatInterval_ = 55;
    int keySpacing_ = 2;
    int borderWidth_ = 1;
    int opacity_ = 100;
    QString keyColor_ = "#202c3d";
    QString borderColor_ = "#101722";
    QString keyLabelColor_ = "#eef2f7";
    QString hoverColor_ = "#3a506d";
    UInputKeyboard uinput_;
    ModifierState shift_ = ModifierState::Off;
    ModifierState ctrl_ = ModifierState::Off;
    ModifierState alt_ = ModifierState::Off;
    ModifierState altGr_ = ModifierState::Off;
    ModifierState meta_ = ModifierState::Off;
    bool capsLock_ = false;
    QPoint dragPointerStart_;
    QPoint dragPanelStart_;
    QPoint panelPosition_;
    QPoint savedPanelPosition_;
    bool restorePanelPosition_ = false;
    LayerShellQt::Window *layer_ = nullptr;
    QTimer layerMoveTimer_;
    QPoint pendingLayerPosition_;

    void applyLayerPosition() {
        if (!layer_)
            return;

        QPoint position = pendingLayerPosition_;
        QScreen *screen = QGuiApplication::screenAt(position);
        if (!screen)
            screen = layer_->screen();
        if (!screen)
            screen = QGuiApplication::primaryScreen();
        if (!screen)
            return;

        const QRect area = screen->availableGeometry();
        position.setX(qBound(area.left(), position.x(),
                             qMax(area.left(), area.right() - width() + 1)));
        position.setY(qBound(area.top(), position.y(),
                             qMax(area.top(), area.bottom() - height() + 1)));
        panelPosition_ = position;
        if (layer_->screen() != screen)
            layer_->setScreen(screen);
        const QPoint localPosition = position - screen->geometry().topLeft();
        layer_->setMargins(QMargins(localPosition.x(), localPosition.y(), 0, 0));
        commitLayerPosition();
    }

    void commitLayerPosition() {
        if (!windowHandle())
            return;
        auto *native = QGuiApplication::platformNativeInterface();
        if (!native)
            return;
        auto *surface = static_cast<wl_surface *>(
            native->nativeResourceForWindow(QByteArrayLiteral("surface"), windowHandle()));
        if (!surface)
            return;

        wl_surface_commit(surface);
        auto *display = static_cast<wl_display *>(
            native->nativeResourceForIntegration(QByteArrayLiteral("display")));
        if (display)
            wl_display_flush(display);
    }

    void movePanel(QPoint position) {
        if (layer_) {
            QScreen *screen = QGuiApplication::screenAt(position);
            if (!screen)
                screen = layer_->screen();
            if (!screen)
                screen = QGuiApplication::primaryScreen();
            if (screen) {
                const QRect area = screen->availableGeometry();
                position.setX(qBound(area.left(), position.x(),
                                     qMax(area.left(), area.right() - width() + 1)));
                position.setY(qBound(area.top(), position.y(),
                                     qMax(area.top(), area.bottom() - height() + 1)));
            }
            panelPosition_ = position;
            pendingLayerPosition_ = position;
            if (!layerMoveTimer_.isActive())
                layerMoveTimer_.start();
        } else {
            move(position);
            panelPosition_ = position;
        }
        panel_->move(0, 0);
    }

    void minimizeToTray() {
        hide();
        tray_->show();
    }

    void restoreFromTray() {
        if (!layer_)
            showNormal();
        show();
        raise();
    }

    void dragTo(const QPoint &pointer, bool begin) {
        if (begin) {
            dragPointerStart_ = pointer;
            dragPanelStart_ = panelPosition_;
            return;
        }
        const QPoint delta = pointer - dragPointerStart_;
        movePanel(dragPanelStart_ + delta);
    }

    QPushButton *key(const QString &label, const QString &action = {}, int width = 1,
                     const QString &inputLabel = {}) {
        auto *button = new KeyButton(label);
        button->setFocusPolicy(Qt::NoFocus);
        button->setAttribute(Qt::WA_MacShowFocusRect, false);
        button->setProperty("special", !action.isEmpty());
        const bool modifier = action == "SHIFT" || action == "CTRL" ||
                              action == "ALT" || action == "RIGHTALT" ||
                              action == "META" || action == "CAPSLOCK";
        button->setModifierKey(modifier, modifierActive(action), modifierLocked(action));
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        const bool repeatable = action.isEmpty() || action == "SPACE" || action == "BACKSPACE" ||
                                action == "DELETE" || action == "RETURN" ||
                                action == "LEFT" || action == "DOWN" ||
                                action == "UP" || action == "RIGHT";
        button->setAutoRepeat(repeatable);
        button->setAutoRepeatDelay(repeatDelay_);
        button->setAutoRepeatInterval(repeatInterval_);

        if (action == "SHIFT") {
            connect(button, &QPushButton::clicked, this, [this] {
                advanceModifier(shift_);
                buildKeys();
            });
        } else if (action == "CAPSLOCK") {
            connect(button, &QPushButton::clicked, this, [this] {
                pressKey("CAPSLOCK");
                capsLock_ = !capsLock_;
                buildKeys();
            });
        } else if (action == "CTRL" || action == "ALT" ||
                   action == "RIGHTALT" || action == "META") {
            connect(button, &QPushButton::clicked, this, [this, action] {
                toggleModifier(action);
                buildKeys();
            });
        } else if (action == "OPTIONS") {
            connect(button, &QPushButton::clicked, this, [this, button] {
                button->setEnabled(false);
                showOptions(button);
            });
        } else if (action == "INSERT") {
            connect(button, &QPushButton::clicked, this, [this, button] {
                button->setEnabled(false);
                showInsertDialog();
                button->setEnabled(true);
            });
        } else if (action == "CUT" || action == "COPY" || action == "PASTE") {
            connect(button, &QPushButton::clicked, this, [this, action] {
                pressClipboardShortcut(action);
            });
        } else if (!action.isEmpty()) {
            connect(button, &QPushButton::clicked, this, [this, action] { pressKey(action); });
        } else {
            const QString input = inputLabel.isEmpty() ? label : inputLabel;
            connect(button, &QPushButton::clicked, this, [this, input] {
                typePrintable(input, modifierActive("SHIFT"));
            });
        }
        if (action == "TAB")
            button->setRightClickAction([this] { sendChord("15", true); clearModifiers(); });
        if (action == "RETURN") {
            button->setRightClickAction([this] { pressShiftEnter(); });
        } else if (action.isEmpty()) {
            const QString input = inputLabel.isEmpty() ? label : inputLabel;
            button->setRightClickAction([this, button, input] {
                button->flashText(shiftedText(input));
                typePrintable(input, true);
            });
        }
        button->setProperty("keyWidth", width);
        return button;
    }

    void clearKeys() {
        while (auto *item = keys_->takeAt(0)) {
            if (auto *row = item->layout())
                while (auto *child = row->takeAt(0))
                    delete child->widget();
            delete item;
        }
    }

    void addRow(const QStringList &labels, const QStringList &actions,
                const QList<int> &widths, const QStringList &inputLabels = {}) {
        auto *row = new QHBoxLayout;
        row->setSpacing(keySpacing_);
        row->setContentsMargins(0, 0, 0, 0);
        for (int i = 0; i < labels.size(); ++i) {
            const QString action = i < actions.size() ? actions[i] : QString();
            const int width = i < widths.size() ? widths[i] : 1;
            const QString input = i < inputLabels.size() ? inputLabels[i] : QString();
            row->addWidget(key(labels[i], action, width, input), width);
        }
        keys_->addLayout(row, 1);
    }

    QString displayPrintable(const QString &label) const {
        return shiftActive() ? shiftedText(label) : label;
    }

    QStringList displayPrintables(const QStringList &labels) const {
        QStringList displayed;
        for (const QString &label : labels)
            displayed << (label.isEmpty() ? label : displayPrintable(label));
        return displayed;
    }

    void buildKeys() {
        clearKeys();
        keys_->setSpacing(keySpacing_);
        if (sideKeys_)
            sideKeys_->setSpacing(keySpacing_);
        if (body_)
            body_->setSpacing(keySpacing_);
        addRow({"Esc","F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12"},
               {"ESC","F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12"},
               {});
        const auto numberRow = QStringList{"`","1","2","3","4","5","6","7","8","9","0","-","=","⌫"};
        addRow(displayPrintables(numberRow),
               {"","","","","","","","","","","","","","BACKSPACE"},
               {1,1,1,1,1,1,1,1,1,1,1,1,1,2}, numberRow);
        const auto topRow = QStringList{"↹","q","w","e","r","t","y","u","i","o","p","[","]","↵"};
        auto insertRow = displayPrintables(topRow);
        insertRow[13] = "Insert";
        addRow(insertRow,
               {"TAB","","","","","","","","","","","","","INSERT"},
               {2,1,1,1,1,1,1,1,1,1,1,1,1,2}, topRow);
        const auto homeRow = QStringList{"⇪","a","s","d","f","g","h","j","k","l",";","'","\\","↵"};
        addRow(displayPrintables(homeRow),
               {"CAPSLOCK","","","","","","","","","","","","","RETURN"},
               {2,1,1,1,1,1,1,1,1,1,1,1,1,2}, homeRow);
        const auto bottomRow = QStringList{"<","z","x","c","v","b","n","m",",",".","/"};
        addRow({modifierActive("SHIFT") ? "⇧•" : "⇧", displayPrintables(bottomRow)[0],
                displayPrintables(bottomRow)[1], displayPrintables(bottomRow)[2],
                displayPrintables(bottomRow)[3], displayPrintables(bottomRow)[4],
                displayPrintables(bottomRow)[5], displayPrintables(bottomRow)[6],
                displayPrintables(bottomRow)[7], displayPrintables(bottomRow)[8],
                displayPrintables(bottomRow)[9], displayPrintables(bottomRow)[10], "⇧"},
               {"SHIFT","","","","","","","","","","","","SHIFT"},
               {2,1,1,1,1,1,1,1,1,1,1,1,2},
               {"SHIFT","<","z","x","c","v","b","n","m",",",".","/","SHIFT"});
        addRow({"Ctrl","Meta","Alt","","Menu","←","↓","↑","→"},
               {"CTRL","META","ALT","SPACE","OPTIONS","LEFT","DOWN","UP","RIGHT"},
               {2,2,2,8,2,1,1,1,1});
    }

    QString shiftedText(const QString &text) const {
        static const QMap<QString, QString> shifted {
            {"`","~"},{"1","!"},{"2","@"},{"3","#"},{"4","$"},{"5","%"},
            {"6","^"},{"7","&"},{"8","*"},{"9","("},{"0",")"},{"-","_"},
            {"=","+"},{"[","{"},{"]","}"},{";",":"},{"'","\""},{"\\","|"},
            {",","<"},{".",">"},{"/","?"},{"<",">"}
        };
        return shifted.value(text, text.toUpper());
    }

    bool modifierActive(const QString &action) const {
        return modifierState(action) != ModifierState::Off;
    }

    bool modifierLocked(const QString &action) const {
        return modifierState(action) == ModifierState::Locked;
    }

    ModifierState modifierState(const QString &action) const {
        if (action == "SHIFT") return shift_;
        if (action == "CTRL") return ctrl_;
        if (action == "ALT") return alt_;
        if (action == "RIGHTALT") return altGr_;
        if (action == "META") return meta_;
        if (action == "CAPSLOCK") return capsLock_ ? ModifierState::Locked : ModifierState::Off;
        return ModifierState::Off;
    }

    void advanceModifier(ModifierState &state) {
        if (state == ModifierState::Off) state = ModifierState::Armed;
        else if (state == ModifierState::Armed) state = ModifierState::Locked;
        else state = ModifierState::Off;
    }

    void clearIfArmed(ModifierState &state) {
        if (state == ModifierState::Armed)
            state = ModifierState::Off;
    }

    void toggleModifier(const QString &action) {
        if (action == "CTRL") advanceModifier(ctrl_);
        else if (action == "ALT") advanceModifier(alt_);
        else if (action == "RIGHTALT") advanceModifier(altGr_);
        else if (action == "META") advanceModifier(meta_);
    }

    void clearModifiers() {
        const bool changed = anyArmedModifier();
        clearIfArmed(shift_);
        clearIfArmed(ctrl_);
        clearIfArmed(alt_);
        clearIfArmed(altGr_);
        clearIfArmed(meta_);
        if (changed)
            buildKeys();
    }

    void clearAllModifiers() {
        shift_ = ctrl_ = alt_ = altGr_ = meta_ = ModifierState::Off;
    }

    bool anyLockedModifier() const {
        return shift_ == ModifierState::Locked || ctrl_ == ModifierState::Locked ||
               alt_ == ModifierState::Locked || altGr_ == ModifierState::Locked ||
               meta_ == ModifierState::Locked;
    }

    bool anyArmedModifier() const {
        return shift_ == ModifierState::Armed || ctrl_ == ModifierState::Armed ||
               alt_ == ModifierState::Armed || altGr_ == ModifierState::Armed ||
               meta_ == ModifierState::Armed;
    }

    bool hasModifier(const ModifierState state) const {
        return state != ModifierState::Off;
    }

    bool shiftActive() const {
        return hasModifier(shift_);
    }

    bool ctrlActive() const {
        return hasModifier(ctrl_);
    }

    bool altActive() const {
        return hasModifier(alt_);
    }

    bool altGrActive() const {
        return hasModifier(altGr_);
    }

    bool metaActive() const {
        return hasModifier(meta_);
    }

    void typePrintable(const QString &label, bool forceShift) {
        static const QMap<QString, QString> codes {
            {"`","41"},{"1","2"},{"2","3"},{"3","4"},{"4","5"},{"5","6"},
            {"6","7"},{"7","8"},{"8","9"},{"9","10"},{"0","11"},{"-","12"},{"=","13"},
            {"q","16"},{"w","17"},{"e","18"},{"r","19"},{"t","20"},{"y","21"},{"u","22"},
            {"i","23"},{"o","24"},{"p","25"},{"[","26"},{"]","27"},
            {"a","30"},{"s","31"},{"d","32"},{"f","33"},{"g","34"},{"h","35"},{"j","36"},
            {"k","37"},{"l","38"},{";","39"},{"'","40"},{"\\","43"},
            {"z","44"},{"x","45"},{"c","46"},{"v","47"},{"b","48"},{"n","49"},{"m","50"},
            {",","51"},{".","52"},{"/","53"},{"<","86"}
        };
        if (activeEditor_) {
            typeText(forceShift || shiftActive() ? shiftedText(label) : label);
            clearModifiers();
            return;
        }
        if (codes.contains(label))
            sendChord(codes[label], forceShift || shiftActive());
        else
            typeText(forceShift || shiftActive() ? shiftedText(label) : label);
        clearModifiers();
    }

    void typeText(const QString &text) {
        if (activeEditor_) {
            activeEditor_->insert(text);
            return;
        }
        queueInput([this, text] {
            for (const uint codepoint : text.toUcs4()) {
                int keyCode = 0;
                bool shifted = false;
                if (UInputKeyboard::asciiKey(codepoint, keyCode, shifted)) {
                    if (!uinput_.tap(keyCode, shifted ? std::vector<int>{KEY_LEFTSHIFT}
                                                       : std::vector<int>{}))
                        return;
                } else if (!uinput_.typeUnicode(codepoint)) {
                    qWarning("Could not type Unicode codepoint U+%04X", codepoint);
                    return;
                }
            }
        });
    }

    void pasteClipboardText(const QString &text) {
        QDBusInterface klipper(QStringLiteral("org.kde.klipper"),
                               QStringLiteral("/klipper"),
                               QStringLiteral("org.kde.klipper.klipper"));
        bool clipboardUpdated = false;
        if (klipper.isValid()) {
            const QDBusMessage result =
                klipper.call(QStringLiteral("setClipboardContents"), text);
            if (result.type() != QDBusMessage::ErrorMessage) {
                const QDBusReply<QString> current =
                    klipper.call(QStringLiteral("getClipboardContents"));
                clipboardUpdated = current.isValid() && current.value() == text;
            }
        }
        if (!clipboardUpdated) {
            qWarning("Klipper did not accept the selected clipboard entry; using Qt clipboard");
            QGuiApplication::clipboard()->setText(text, QClipboard::Clipboard);
        }

        QTimer::singleShot(200, this, [this] {
            queueInput([this] {
                uinput_.tap(KEY_V, {KEY_LEFTCTRL});
                clearModifiers();
            });
        });
    }

    void showInsertDialog(bool preload = false) {
        insertTargetWindow_ = QGuiApplication::focusWindow();
        if (insertDialogInitialized_) {
            if (!preload) {
                insertDialog_.setAttribute(Qt::WA_ShowWithoutActivating, false);
                insertDialog_.show();
                insertDialog_.raise();
                insertDialog_.activateWindow();
            }
            return;
        }
        QDialog &dialog = insertDialog_;
        dialog.setWindowTitle("Insert");
        // Let the window manager handle activation and focus handoff to other
        // applications. Override-redirect windows bypass that coordination.
        dialog.setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint |
                              Qt::FramelessWindowHint);
        dialog.setAttribute(Qt::WA_ShowWithoutActivating, true);
        dialog.setFocusPolicy(Qt::StrongFocus);
        dialog.setContentsMargins(0, 0, 0, 0);
        connect(qApp, &QGuiApplication::focusWindowChanged, &dialog,
                [this, &dialog](QWindow *window) {
                    if (window != dialog.windowHandle())
                        activeEditor_ = nullptr;
                });
        dialog.setStyleSheet(QString(R"(
            QDialog {
                padding: 0;
                background: %2;
                border: 1px solid #26384f;
                border-radius: 8px;
            }
            QLabel { background: transparent; color: #eef2f7; }
            #dragBar {
                background: %1;
                color: #607087;
                border: 1px solid %2;
                border-radius: 4px;
                font: 9px "Noto Sans";
            }
            QListWidget, QLineEdit {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                            stop:0 %1, stop:1 %1);
                color: #ffffff;
                border: 1px solid %2;
                border-radius: 4px;
                selection-background-color: #5576a3;
                selection-color: #ffffff;
                padding: 4px;
            }
            QListWidget::item { color: #ffffff; padding: 4px; }
            QListWidget::item:hover { background: #2b3a50; }
            QPushButton {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                            stop:0 %1, stop:1 %1);
                color: #ffffff;
                border: 1px solid %2;
                border-radius: 4px;
                padding: 5px 10px;
            }
            QPushButton:hover {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                            stop:0 #3a506d, stop:1 #263a54);
                border-color: #6b8db7;
            }
            QPushButton:pressed { background: #3a4c65; }
            QPushButton:disabled {
                background: #101722;
                color: #667386;
                border-color: #1a2635;
            }
            QPushButton#closeButton {
                background: #151f2d; color: #9eabbc;
                border: 1px solid #101722; border-radius: 3px;
                padding: 0; font: 15px "Noto Sans";
            }
            QPushButton#closeButton:hover { background: #a53b48; color: white; }
            QPushButton#closeButton:pressed { background: #cf4d5d; color: white; }
        )").arg(keyColor_, borderColor_));
        settings_.beginGroup("insertWindow");
        const QByteArray savedInsertGeometry = settings_.value("geometry").toByteArray();
        int insertWidth = settings_.value("width", 620).toInt();
        int insertHeight = settings_.value("height", 420).toInt();
        const bool hasInsertPosition = settings_.contains("x") && settings_.contains("y");
        const QPoint savedInsertPosition(settings_.value("x", 0).toInt(),
                                         settings_.value("y", 0).toInt());
        const QSize savedInsertSize(qMax(420, insertWidth), qMax(240, insertHeight));
        dialog.resize(savedInsertSize);
        settings_.endGroup();
        auto *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(4, 4, 4, 4);
        auto *titleBar = new QHBoxLayout;
        titleBar->setContentsMargins(0, 0, 0, 0);
        titleBar->addWidget(new DragBar(&dialog, [this, &dialog](QPoint pointer, bool begin) {
            if (begin) {
                insertDragPointer_ = pointer;
                insertDragPosition_ = dialog.pos();
            } else {
                dialog.move(insertDragPosition_ + pointer - insertDragPointer_);
            }
        }, QGuiApplication::platformName() == QStringLiteral("wayland")), 1);
        auto *close = new QPushButton("×", &dialog);
        close->setObjectName("closeButton");
        close->setFixedSize(22, 14);
        close->setStyleSheet(QString("QPushButton { background: %1; color: #eef2f7; font: bold 10px 'Noto Sans'; padding: 0; margin: 0; min-height: 0; max-height: 14px; }").arg(keyColor_));
        close->setToolTip("Close Insert window");
        titleBar->addWidget(close);
        auto *insertBarSpacer = new QWidget(&dialog);
        insertBarSpacer->setFixedWidth(20);
        insertBarSpacer->setStyleSheet(QString("background: %1;").arg(keyColor_));
        titleBar->addWidget(insertBarSpacer);
        layout->addLayout(titleBar);
        QObject::connect(close, &QPushButton::clicked, &dialog, [this, &dialog] {
            const QSize size = dialog.size();
            settings_.beginGroup("insertWindow");
            settings_.setValue("width", size.width());
            settings_.setValue("height", size.height());
            settings_.setValue("x", dialog.x());
            settings_.setValue("y", dialog.y());
            settings_.setValue("geometry", dialog.saveGeometry());
            settings_.endGroup();
            settings_.sync();
            dialog.reject();
        });
        QObject::connect(&dialog, &QDialog::finished, this, [this, &dialog] {
            settings_.beginGroup("insertWindow");
            settings_.setValue("width", dialog.width());
            settings_.setValue("height", dialog.height());
            settings_.setValue("x", dialog.x());
            settings_.setValue("y", dialog.y());
            settings_.setValue("geometry", dialog.saveGeometry());
            settings_.endGroup();
            settings_.sync();
        });
        auto *columns = new QHBoxLayout;
        auto *snippetsColumn = new QVBoxLayout;
        auto *clipboardColumn = new QVBoxLayout;
        snippetsColumn->addWidget(new QLabel("Snippets", &dialog));
        clipboardColumn->addWidget(new QLabel("Clipboard", &dialog));
        auto filterClicked = [this](QLineEdit *field) {
            activeEditor_ = field;
        };
        auto filterFocusLost = [this] {
            activeEditor_ = nullptr;
        };
        auto *snippetFilter = new FocusLineEdit(&dialog, filterClicked, filterFocusLost);
        auto *clipboardFilter = new FocusLineEdit(&dialog, filterClicked, filterFocusLost);
        snippetFilter->setPlaceholderText("Filter snippets");
        clipboardFilter->setPlaceholderText("Filter clipboard");
        snippetsColumn->addWidget(snippetFilter);
        clipboardColumn->addWidget(clipboardFilter);
        auto *snippetList = new QListWidget(&dialog);
        auto *clipboardList = new QListWidget(&dialog);
        for (auto *list : {snippetList, clipboardList}) {
            list->setWordWrap(false);
            list->setTextElideMode(Qt::ElideRight);
            list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            list->setAttribute(Qt::WA_AlwaysShowToolTips, true);
        }
        const auto preview = [](const QString &value) {
            return value.size() > 34 ? value.left(31) + QStringLiteral("…") : value;
        };
        snippetsColumn->addWidget(snippetList);
        clipboardColumn->addWidget(clipboardList);
        columns->addLayout(snippetsColumn);
        columns->addLayout(clipboardColumn);
        layout->addLayout(columns);

        auto *editor = new FocusLineEdit(&dialog, filterClicked, filterFocusLost);
        editor->setPlaceholderText("New snippet text");
        auto *editorRow = new QHBoxLayout;
        auto *edit = new QPushButton("Update", &dialog);
        edit->setToolTip("Update the selected snippet with this text");
        editorRow->addWidget(editor, 1);
        editorRow->addWidget(edit);
        layout->addLayout(editorRow);
        auto *keepOpen = new QCheckBox("Keep open after inserting", &dialog);
        layout->addWidget(keepOpen);
        auto *buttons = new QHBoxLayout;
        auto *add = new QPushButton("Add", &dialog);
        auto *cancel = new QPushButton("Cancel", &dialog);
        auto *remove = new QPushButton("Remove", &dialog);
        auto *insert = new QPushButton("Insert", &dialog);
        add->setEnabled(false);
        cancel->setEnabled(false);
        edit->setEnabled(false);
        remove->setEnabled(false);
        buttons->addWidget(add);
        buttons->addWidget(cancel);
        buttons->addWidget(remove);
        buttons->addStretch();
        buttons->addWidget(insert);
        layout->addLayout(buttons);
        const auto insertResize = [&dialog, this](QPoint size) {
            dialog.resize(size.x(), size.y());
            settings_.beginGroup("insertWindow");
            settings_.setValue("width", dialog.width());
            settings_.setValue("height", dialog.height());
            settings_.setValue("geometry", dialog.saveGeometry());
            settings_.endGroup();
            settings_.sync();
        };
        new ResizeGrip(&dialog, insertResize, false, Qt::TopEdge | Qt::LeftEdge);
        new ResizeGrip(&dialog, insertResize, false, Qt::TopEdge | Qt::RightEdge);
        new ResizeGrip(&dialog, insertResize, false, Qt::BottomEdge | Qt::LeftEdge);
        new ResizeGrip(&dialog, insertResize, false, Qt::BottomEdge | Qt::RightEdge);

        auto *snippetValues = &snippetValues_;
        settings_.beginGroup("snippets");
        *snippetValues = settings_.value("items").toStringList();
        settings_.endGroup();
        for (const auto &snippet : *snippetValues) {
            auto *item = new QListWidgetItem(preview(snippet), snippetList);
            item->setToolTip(snippet);
        }

        auto applyClipboardFilter = [clipboardFilter, clipboardList] {
            const QString query = clipboardFilter->text().trimmed();
            for (int i = 0; i < clipboardList->count(); ++i)
                clipboardList->item(i)->setHidden(
                    !query.isEmpty() &&
                    !clipboardList->item(i)->toolTip().contains(query, Qt::CaseInsensitive));
        };
        auto lastClipboardValues = std::make_shared<QStringList>();
        auto loadClipboardHistory = [clipboardList, preview, applyClipboardFilter,
                                     lastClipboardValues] {
            QDBusInterface klipper(QStringLiteral("org.kde.klipper"),
                                   QStringLiteral("/klipper"),
                                   QStringLiteral("org.kde.klipper.klipper"));
            if (!klipper.isValid())
                return;
            const QDBusReply<QStringList> reply =
                klipper.call(QStringLiteral("getClipboardHistoryMenu"));
            if (!reply.isValid())
                return;
            const QStringList history = reply.value();
            QStringList values;
            const QRegularExpression imageDimensions(
                QStringLiteral("(^|\\s)\\d+\\s*[x×]\\s*\\d+(\\s|$)"));
            const auto isImageEntry = [&imageDimensions](const QString &entry) {
                const QString label = entry.trimmed();
                return label.startsWith(QStringLiteral("▨")) ||
                       label.contains(imageDimensions);
            };
            for (int historyIndex = 0; historyIndex < history.size(); ++historyIndex) {
                const QString &historyEntry = history.at(historyIndex);
                // Klipper represents image entries as labels such as "▨ 303x537".
                // They are not text clipboard entries and must not be shown here.
                if (isImageEntry(historyEntry))
                    continue;
                // getClipboardHistoryMenu() is a display-oriented API. Fetch the
                // actual entry so leading/trailing whitespace and multiline text
                // survive exactly as they were copied.
                QString value = historyEntry;
                const QDBusReply<QString> itemReply =
                    klipper.call(QStringLiteral("getClipboardHistoryItem"), historyIndex);
                if (itemReply.isValid())
                    value = itemReply.value();
                if (value.isEmpty()) continue;
                if (isImageEntry(value))
                    continue;
                values.append(value);
            }
            QString selectedValue;
            if (clipboardList->currentItem())
                selectedValue = clipboardList->currentItem()->toolTip();
            if (values == *lastClipboardValues) {
                applyClipboardFilter();
                return;
            }
            *lastClipboardValues = values;
            clipboardList->clear();
            for (const QString &value : values) {
                auto *item = new QListWidgetItem(preview(value), clipboardList);
                item->setToolTip(value);
            }
            applyClipboardFilter();
            if (!selectedValue.isEmpty()) {
                for (int row = 0; row < clipboardList->count(); ++row) {
                    if (clipboardList->item(row)->toolTip() == selectedValue) {
                        clipboardList->setCurrentRow(row);
                        break;
                    }
                }
            }
        };
        auto *clipboardRefreshTimer = new QTimer(&dialog);
        clipboardRefreshTimer->setInterval(100);
        clipboardRefreshTimer->setSingleShot(true);
        connect(clipboardRefreshTimer, &QTimer::timeout, &dialog, loadClipboardHistory);
        const bool klipperSignalConnected = QDBusConnection::sessionBus().connect(
            QStringLiteral("org.kde.klipper"), QStringLiteral("/klipper"),
            QStringLiteral("org.kde.klipper.klipper"),
            QStringLiteral("clipboardHistoryUpdated"), clipboardRefreshTimer,
            SLOT(start()));
        if (!klipperSignalConnected) {
            clipboardRefreshTimer->setInterval(1000);
            clipboardRefreshTimer->setSingleShot(false);
            clipboardRefreshTimer->start();
        }
        if (preload)
            QTimer::singleShot(1000, &dialog, loadClipboardHistory);
        else
            loadClipboardHistory();

        auto filterList = [](QLineEdit *filter, QListWidget *list) {
            QObject::connect(filter, &QLineEdit::textChanged, list, [filter, list] {
                const QString query = filter->text().trimmed();
                for (int i = 0; i < list->count(); ++i)
                    list->item(i)->setHidden(!query.isEmpty() &&
                                              !list->item(i)->toolTip().contains(query, Qt::CaseInsensitive));
            });
        };
        filterList(snippetFilter, snippetList);
        connect(clipboardFilter, &QLineEdit::textChanged, &dialog,
                [applyClipboardFilter](const QString &) { applyClipboardFilter(); });
        for (auto *list : {snippetList, clipboardList}) {
            list->setMouseTracking(true);
            connect(list, &QListWidget::itemEntered, &dialog,
                    [list](QListWidgetItem *item) {
                        if (item && !item->toolTip().isEmpty())
                            QToolTip::showText(QCursor::pos(), item->toolTip(), list);
                    });
        }

        connect(snippetList, &QListWidget::currentRowChanged, &dialog, [editor, snippetValues](int row) {
            if (row >= 0 && row < snippetValues->size()) editor->setText(snippetValues->at(row));
        });
        connect(snippetList, &QListWidget::currentRowChanged, &dialog,
                [clipboardList](int row) {
                    if (row >= 0 && clipboardList->currentRow() >= 0)
                        clipboardList->setCurrentRow(-1);
                });
        connect(clipboardList, &QListWidget::currentRowChanged, &dialog,
                [snippetList](int row) {
                    if (row >= 0 && snippetList->currentRow() >= 0)
                        snippetList->setCurrentRow(-1);
                });
        connect(add, &QPushButton::clicked, &dialog, [&dialog, snippetList, editor, snippetValues, this, preview] {
            if (editor->text().isEmpty()) return;
            snippetValues->append(editor->text());
            auto *item = new QListWidgetItem(preview(editor->text()), snippetList);
            item->setToolTip(editor->text());
            settings_.beginGroup("snippets"); settings_.setValue("items", *snippetValues); settings_.endGroup();
            editor->clear();
            activeEditor_ = nullptr;
            if (insertTargetWindow_)
                insertTargetWindow_->requestActivate();
        });
        QPointer<QPushButton> addButton = add;
        QPointer<QPushButton> cancelButton = cancel;
        connect(editor, &QLineEdit::textChanged, &dialog,
                [addButton, cancelButton](const QString &text) {
                    const bool hasText = !text.trimmed().isEmpty();
                    QTimer::singleShot(0, [addButton, cancelButton, hasText] {
                        if (addButton) addButton->setEnabled(hasText);
                        if (cancelButton) cancelButton->setEnabled(hasText);
                    });
                });
        const auto refreshUpdate = [edit, editor, snippetList] {
            edit->setEnabled(snippetList->currentRow() >= 0 &&
                             !editor->text().trimmed().isEmpty());
        };
        connect(editor, &QLineEdit::textChanged, &dialog, refreshUpdate);
        connect(snippetList, &QListWidget::currentRowChanged, &dialog, refreshUpdate);
        connect(cancel, &QPushButton::clicked, &dialog,
                [cancel, editor, this] {
                    editor->clear();
                    cancel->setEnabled(false);
                    activeEditor_ = nullptr;
                    if (insertTargetWindow_)
                        insertTargetWindow_->requestActivate();
        });
        connect(snippetList, &QListWidget::currentRowChanged, &dialog,
                [remove](int row) { remove->setEnabled(row >= 0); });
        connect(edit, &QPushButton::clicked, &dialog, [snippetList, editor, snippetValues, this, preview] {
            const int row = snippetList->currentRow();
            if (row < 0 || row >= snippetValues->size() || editor->text().isEmpty()) return;
            (*snippetValues)[row] = editor->text();
            snippetList->item(row)->setText(preview(editor->text()));
            snippetList->item(row)->setToolTip(editor->text());
            settings_.beginGroup("snippets"); settings_.setValue("items", *snippetValues); settings_.endGroup();
        });
        connect(remove, &QPushButton::clicked, &dialog, [snippetList, snippetValues, this] {
            const int row = snippetList->currentRow();
            if (row < 0 || row >= snippetValues->size()) return;
            snippetValues->removeAt(row); delete snippetList->takeItem(row);
            settings_.beginGroup("snippets"); settings_.setValue("items", *snippetValues); settings_.endGroup();
        });
        connect(insert, &QPushButton::clicked, &dialog,
                [this, &dialog, snippetList, clipboardList, snippetValues, keepOpen] {
            QString value;
            bool fromClipboard = false;
            if (snippetList->currentRow() >= 0)
                value = snippetValues->at(snippetList->currentRow());
            else if (clipboardList->currentRow() >= 0) {
                // Keep the value captured when the list was built. Klipper's
                // history indices can shift while the filtered dialog stays open.
                value = clipboardList->currentItem()->toolTip();
                fromClipboard = true;
            }
            if (!value.isEmpty()) {
                if (insertTargetWindow_)
                    insertTargetWindow_->requestActivate();
                QTimer::singleShot(150, this, [this, value, fromClipboard] {
                    if (fromClipboard)
                        pasteClipboardText(value);
                    else
                        typeText(value);
                });
            }
            if (!keepOpen->isChecked()) {
                const QSize size = dialog.size();
                settings_.beginGroup("insertWindow");
                settings_.setValue("width", size.width());
                settings_.setValue("height", size.height());
                settings_.setValue("geometry", dialog.saveGeometry());
                settings_.endGroup();
                settings_.sync();
                dialog.done(value.isEmpty() ? QDialog::Rejected : QDialog::Accepted);
            }
        });
        connect(snippetList, &QListWidget::itemDoubleClicked, insert,
                [insert](QListWidgetItem *) { insert->click(); });
        connect(clipboardList, &QListWidget::itemDoubleClicked, insert,
                [insert](QListWidgetItem *) { insert->click(); });
        connect(&dialog, &QDialog::finished, this, [this, &dialog] {
            const QSize size = dialog.size();
            settings_.beginGroup("insertWindow");
            settings_.setValue("width", size.width());
            settings_.setValue("height", size.height());
            settings_.setValue("x", dialog.x());
            settings_.setValue("y", dialog.y());
            settings_.setValue("geometry", dialog.saveGeometry());
            settings_.endGroup();
            settings_.sync();
        });
        connect(&dialog, &QDialog::finished, this, [this] {
            activeEditor_ = nullptr;
            if (insertTargetWindow_)
                insertTargetWindow_->requestActivate();
        });
        dialog.resize(savedInsertSize);
        if (hasInsertPosition)
            dialog.move(savedInsertPosition);
        else if (!savedInsertGeometry.isEmpty())
            dialog.restoreGeometry(savedInsertGeometry);
        dialog.setWindowModality(Qt::NonModal);
        insertDialogInitialized_ = true;
        if (!preload) {
            dialog.show();
            dialog.raise();
        }
    }

    void pressKey(const QString &name) {
        static const QMap<QString, QString> codes {
            {"ESC","1"},{"BACKSPACE","14"},{"TAB","15"},{"RETURN","28"},
            {"CTRL","29"},{"SHIFT","42"},{"ALT","56"},{"CAPSLOCK","58"},
            {"F1","59"},{"F2","60"},{"F3","61"},{"F4","62"},{"F5","63"},
            {"F6","64"},{"F7","65"},{"F8","66"},{"F9","67"},{"F10","68"},
            {"F11","87"},{"F12","88"},{"META","125"},{"RIGHTALT","100"},
            {"MENU","127"},{"UP","103"},{"DOWN","108"},{"LEFT","105"},{"RIGHT","106"},
            {"HOME","102"},{"END","107"},{"DELETE","111"},{"SPACE","57"}
        };
        if (activeEditor_ && name == QStringLiteral("SPACE")) {
            activeEditor_->insert(QStringLiteral(" "));
            clearModifiers();
            return;
        }
        if (codes.contains(name))
            sendChord(codes[name], shiftActive());
        clearModifiers();
    }

    void pressShiftEnter() {
        sendChord("28", true);
        clearModifiers();
    }

    void pressClipboardShortcut(const QString &action) {
        static const QMap<QString, QString> codes {
            {"CUT", "45"}, {"COPY", "46"}, {"PASTE", "47"}
        };
        if (!codes.contains(action))
            return;
        const int keyCode = codes[action].toInt();
        queueInput([this, keyCode] {
            uinput_.tap(keyCode, {KEY_LEFTCTRL});
            clearModifiers();
        });
    }

    void sendChord(const QString &keyCode, bool includeShift) {
        std::vector<int> modifiers;
        if (ctrlActive()) modifiers.push_back(KEY_LEFTCTRL);
        if (includeShift) modifiers.push_back(KEY_LEFTSHIFT);
        if (altActive()) modifiers.push_back(KEY_LEFTALT);
        if (altGrActive()) modifiers.push_back(KEY_RIGHTALT);
        if (metaActive()) modifiers.push_back(KEY_LEFTMETA);
        const int code = keyCode.toInt();
        queueInput([this, code, modifiers = std::move(modifiers)]() mutable {
            uinput_.tap(code, modifiers);
        });
    }

    void queueInput(std::function<void()> input) {
        if (QGuiApplication::platformName() != QStringLiteral("xcb")) {
            input();
            return;
        }
        QTimer::singleShot(25, this, [this, input = std::move(input)]() mutable {
            input();
        });
    }

    void saveSettings() {
        settings_.beginGroup("appearance");
        settings_.setValue("repeatDelay", repeatDelay_);
        settings_.setValue("repeatInterval", repeatInterval_);
        settings_.setValue("keySpacing", keySpacing_);
        settings_.setValue("borderWidth", borderWidth_);
        settings_.setValue("opacity", opacity_);
        settings_.setValue("keyColor", keyColor_);
        settings_.setValue("borderColor", borderColor_);
        settings_.setValue("keyLabelColor", keyLabelColor_);
        settings_.setValue("hoverColor", hoverColor_);
        settings_.endGroup();
        settings_.sync();
    }

    void refreshKeyboard(bool rebuild = false) {
        if (rebuild)
            buildKeys();
        applyStyle();
        opacityEffect_->setOpacity(opacity_ / 100.0);
        saveSettings();
    }

    void showOptions(QPushButton *menuButton) {
        auto *dialog = new QDialog;
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint |
                               Qt::X11BypassWindowManagerHint);
        dialog->setAttribute(Qt::WA_ShowWithoutActivating, true);
        dialog->setContentsMargins(0, 0, 0, 0);
        settings_.beginGroup("menuWindow");
        const QByteArray menuGeometry = settings_.value("geometry").toByteArray();
        settings_.endGroup();
        constexpr int menuWidth = 520;
        constexpr int menuHeight = 700;
        dialog->setFixedSize(menuWidth, menuHeight);
        dialog->setStyleSheet(QString(R"(
            QDialog { padding: 0; background: %2;
                color: #eef2f7; border: 1px solid #26384f; border-radius: 8px; }
            QLabel { color: #eef2f7; font-size: 16px; padding-top: 10px; }
            #dragBar { background: %1; color: #607087; border: 1px solid %2;
                border-radius: 4px; font: 9px "Noto Sans"; }
            QPushButton#closeButton {
                background: #151f2d; color: #9eabbc;
                border: 1px solid #101722; border-radius: 3px;
                padding: 0; font: 15px "Noto Sans";
            }
            QPushButton#closeButton:hover { background: #a53b48; color: white; }
            QPushButton#closeButton:pressed { background: #cf4d5d; color: white; }
            QSpinBox, QSlider, QPushButton {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 %1, stop:1 %1);
                color: #ffffff; border: 1px solid %2; border-radius: 4px; padding: 7px 10px;
                min-height: 34px; font-size: 15px; margin-bottom: 8px;
            }
            QPushButton:hover { background: #2b3a50; border-color: #6b8db7; }
            QPushButton:pressed { background: #3a4c65; }
            QSlider::groove:horizontal { background: #101722; height: 6px; border-radius: 3px; }
            QSlider::handle:horizontal { background: #6b8db7; width: 14px; margin: -5px 0; border-radius: 7px; }
        )").arg(keyColor_, borderColor_));
        auto *outer = new QVBoxLayout(dialog);
        outer->setContentsMargins(0, 0, 0, 0);
        outer->setSpacing(0);
        auto menuDragPointer = std::make_shared<QPoint>();
        auto menuDragPosition = std::make_shared<QPoint>();
        auto *titleBar = new QHBoxLayout;
        titleBar->setContentsMargins(0, 0, 0, 0);
        titleBar->setSpacing(1);
        titleBar->addWidget(new DragBar(dialog, [dialog, menuDragPointer,
                                                  menuDragPosition](QPoint pointer, bool begin) {
            if (begin) {
                *menuDragPointer = pointer;
                *menuDragPosition = dialog->pos();
            } else {
                dialog->move(*menuDragPosition + pointer - *menuDragPointer);
            }
        }, QGuiApplication::platformName() == QStringLiteral("wayland")), 1);
        auto *close = new QPushButton("×", dialog);
        close->setObjectName("closeButton");
        close->setFixedSize(22, 14);
        close->setStyleSheet(QString("QPushButton { background: %1; color: #eef2f7; font: bold 10px 'Noto Sans'; padding: 0; margin: 0; min-height: 0; max-height: 14px; }").arg(keyColor_));
        titleBar->addWidget(close);
        auto *menuBarSpacer = new QWidget(dialog);
        menuBarSpacer->setFixedWidth(20);
        menuBarSpacer->setStyleSheet(QString("background: %1;").arg(keyColor_));
        titleBar->addWidget(menuBarSpacer);
        outer->addLayout(titleBar);
        QObject::connect(close, &QPushButton::clicked, dialog, &QDialog::close);
        auto *form = new QFormLayout;
        form->setContentsMargins(30, 18, 14, 14);
        form->setHorizontalSpacing(14);
        form->setVerticalSpacing(36);
        form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
        outer->addLayout(form);

        auto spin = [dialog](int value, int minimum, int maximum, const QString &suffix) {
            auto *control = new QSpinBox(dialog);
            control->setRange(minimum, maximum);
            control->setValue(value);
            control->setSuffix(suffix);
            return control;
        };
        auto *delay = spin(repeatDelay_, 100, 1500, " ms");
        auto *interval = spin(repeatInterval_, 15, 500, " ms");
        auto *spacing = spin(keySpacing_, 0, 12, " px");
        auto *border = spin(borderWidth_, 0, 6, " px");
        auto *opacity = new QSlider(Qt::Horizontal, dialog);
        opacity->setRange(20, 100);
        opacity->setValue(opacity_);
        auto *keyColor = new QPushButton(keyColor_, dialog);
        auto *borderColor = new QPushButton(borderColor_, dialog);
        auto *keyLabelColor = new QPushButton(keyLabelColor_, dialog);
        auto *hoverColor = new QPushButton(hoverColor_, dialog);
        const auto updateColorButton = [](QPushButton *button, const QString &color) {
            QPixmap swatch(26, 26);
            swatch.fill(QColor(color));
            button->setIcon(QIcon(swatch));
            button->setIconSize(QSize(26, 26));
            button->setText("   " + color);
        };
        updateColorButton(keyColor, keyColor_);
        updateColorButton(borderColor, borderColor_);
        updateColorButton(keyLabelColor, keyLabelColor_);
        updateColorButton(hoverColor, hoverColor_);
        keyColor->setMinimumWidth(140);
        borderColor->setMinimumWidth(140);
        keyLabelColor->setMinimumWidth(140);
        hoverColor->setMinimumWidth(140);
        const auto chooseColor = [dialog](const QColor &initial, const QString &title) {
            QColorDialog picker(initial, dialog);
            picker.setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint |
                                  Qt::X11BypassWindowManagerHint);
            picker.setWindowModality(Qt::WindowModal);
            picker.setWindowTitle(title);
            picker.show();
            picker.raise();
            picker.activateWindow();
            return picker.exec() == QDialog::Accepted ? picker.selectedColor() : QColor();
        };

        form->addRow("Repeat delay", delay);
        form->addRow("Repeat interval", interval);
        form->addRow("Opacity", opacity);
        form->addRow("Key spacing", spacing);
        form->addRow("Border width", border);
        form->addRow("Key color", keyColor);
        form->addRow("Border color", borderColor);
        form->addRow("Key label color", keyLabelColor);
        form->addRow("Key hover color", hoverColor);

        connect(delay, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
            repeatDelay_ = value; refreshKeyboard(true);
        });
        connect(interval, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
            repeatInterval_ = value; refreshKeyboard(true);
        });
        connect(spacing, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
            keySpacing_ = value; refreshKeyboard(true);
        });
        connect(border, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
            borderWidth_ = value; refreshKeyboard();
        });
        connect(opacity, &QSlider::valueChanged, this, [this](int value) {
            opacity_ = value; refreshKeyboard();
        });
        connect(keyColor, &QPushButton::clicked, dialog, [this, dialog, keyColor, chooseColor, updateColorButton] {
            const QColor color = chooseColor(QColor(keyColor_), "Key color");
            if (color.isValid()) { keyColor_ = color.name(); updateColorButton(keyColor, keyColor_); refreshKeyboard(); }
        });
        connect(borderColor, &QPushButton::clicked, dialog, [this, dialog, borderColor, chooseColor, updateColorButton] {
            const QColor color = chooseColor(QColor(borderColor_), "Border color");
            if (color.isValid()) { borderColor_ = color.name(); updateColorButton(borderColor, borderColor_); refreshKeyboard(); }
        });
        connect(keyLabelColor, &QPushButton::clicked, dialog, [this, dialog, keyLabelColor, chooseColor, updateColorButton] {
            const QColor color = chooseColor(QColor(keyLabelColor_), "Key label color");
            if (color.isValid()) { keyLabelColor_ = color.name(); updateColorButton(keyLabelColor, keyLabelColor_); refreshKeyboard(); }
        });
        connect(hoverColor, &QPushButton::clicked, dialog, [this, dialog, hoverColor, chooseColor, updateColorButton] {
            const QColor color = chooseColor(QColor(hoverColor_), "Key hover color");
            if (color.isValid()) { hoverColor_ = color.name(); updateColorButton(hoverColor, hoverColor_); refreshKeyboard(); }
        });

        if (!menuGeometry.isEmpty())
            dialog->restoreGeometry(menuGeometry);
        connect(dialog, &QDialog::finished, this, [this, dialog] {
            settings_.beginGroup("menuWindow");
            settings_.setValue("width", dialog->width());
            settings_.setValue("height", dialog->height());
            settings_.setValue("geometry", dialog->saveGeometry());
            settings_.endGroup();
            settings_.sync();
        });
        connect(dialog, &QDialog::destroyed, menuButton, [menuButton] {
            if (menuButton)
                menuButton->setEnabled(true);
        });
        dialog->show();
        dialog->raise();
        QTimer::singleShot(0, dialog, [dialog] { dialog->raise(); });
        QTimer::singleShot(0, dialog, [dialog] { dialog->raise(); });
    }

    void applyStyle() {
        const QString style = QString(R"(
            #keyboard { background: transparent; }
            #keyboardPanel {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                            stop:0 #172333, stop:0.45 #0f1825, stop:1 #0a1018);
                border: 1px solid #26384f;
                border-radius: 8px;
            }
            #dragBar {
                background: %1;
                color: #607087;
                border: %2px solid %3;
                border-radius: 3px;
                font: 9px "Noto Sans";
            }
            #closeButton {
                background: %1;
                color: %4;
                border: %2px solid %3;
                border-radius: 3px;
                padding: 0;
                font: 12px "Noto Sans";
            }
            #closeButton:hover { background: #a53b48; color: white; }
            #closeButton:pressed { background: #cf4d5d; color: white; }
            #monitorButton, #minimizeButton, #topBarSpacer {
                background: %1;
                border: %2px solid %3;
                color: %4;
            }
            QPushButton {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                            stop:0 %1, stop:1 %7);
                color: %4;
                border: %2px solid %3;
                border-radius: 4px;
                padding: 0px 2px;
                font: 14px "Noto Sans";
            }
            QPushButton:hover {
                background: %5;
                border-color: #6b8db7;
            }
            QPushButton:pressed {
                background: %6;
                padding-top: 2px;
            }
            QPushButton[special="true"] { color: %4; font-size: 12px; }
            QPushButton[clipboardAction="CUT"],
            QPushButton[clipboardAction="COPY"],
            QPushButton[clipboardAction="PASTE"] { font-size: 18px; }
            QPushButton[modifierPressed="true"],
            QPushButton[activeModifier="true"] {
                background: #5576a3;
                color: white;
                border-color: #8db8ef;
            }
            QPushButton[lockedModifier="true"] {
                background: #9b3441;
                color: white;
                border-color: #f0808d;
            }
        )").arg(keyColor_).arg(borderWidth_).arg(borderColor_).arg(keyLabelColor_)
             .arg(hoverColor_).arg(QColor(keyColor_).darker(145).name())
             .arg(QColor(keyColor_).darker(165).name());
        setStyleSheet(style);
        opacityEffect_->setOpacity(opacity_ / 100.0);
    }
};

int main(int argc, char **argv) {
    // Prefer Xwayland when it is available. Its override-redirect surface stays
    // outside the compositor focus chain; DragBar moves it directly. The native
    // Wayland fallback uses layer-shell and is intended for sessions without
    // Xwayland.
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM",
                qEnvironmentVariableIsEmpty("DISPLAY") ? "wayland" : "xcb");
    }
    if (qEnvironmentVariable("QT_QPA_PLATFORM") == QStringLiteral("wayland") &&
        qEnvironmentVariableIsEmpty("QT_WAYLAND_SHELL_INTEGRATION")) {
        qputenv("QT_WAYLAND_SHELL_INTEGRATION", "layer-shell");
    }
    QApplication app(argc, argv);
    app.setApplicationName("orbit-osk");
    app.setOrganizationName("Orbit");
    app.setQuitOnLastWindowClosed(false);
    const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    const QString lockPath = (runtimeDir.isEmpty() ? QDir::tempPath() : runtimeDir)
                             + "/orbit-osk.lock";
    QLockFile instanceLock(lockPath);
    instanceLock.setStaleLockTime(0);
    if (!instanceLock.tryLock(100)) {
        qint64 previousPid = 0;
        QString previousHost;
        QString previousApp;

        // Replace an existing local instance when launched again. Only use
        // the PID recorded by our lock file; never scan or kill by name.
        if (instanceLock.getLockInfo(&previousPid, &previousHost, &previousApp) &&
            previousApp == QStringLiteral("orbit-osk") && previousPid > 0 &&
            previousPid != static_cast<qint64>(getpid())) {
            if (kill(static_cast<pid_t>(previousPid), SIGTERM) == 0 || errno == ESRCH) {
                for (int i = 0; i < 20 && !instanceLock.tryLock(50); ++i)
                    QThread::msleep(50);
            }
        }

        if (!instanceLock.isLocked() && !instanceLock.tryLock(100))
            return 0;
    }
    Keyboard keyboard;
    keyboard.show();
    return app.exec();
}
