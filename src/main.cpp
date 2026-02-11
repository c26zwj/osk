#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QAction>

#include <LayerShellQt/Shell>
#include <LayerShellQt/Window>
#include <KStatusNotifierItem>
#include <KGlobalAccel>

#include "keyboardcontroller.h"

int main(int argc, char *argv[])
{
    // Must be called before QApplication — enables layer-shell for windows
    LayerShellQt::Shell::useLayerShell();

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("osk"));
    app.setApplicationDisplayName(QStringLiteral("On-Screen Keyboard"));
    app.setDesktopFileName(QStringLiteral("osk"));

    auto *controller = new KeyboardController(&app);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("KeyboardController"), controller);
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    if (!window)
        return -1;

    // Configure layer shell — fullscreen transparent overlay
    auto *lsWindow = LayerShellQt::Window::get(window);
    lsWindow->setLayer(LayerShellQt::Window::LayerOverlay);
    lsWindow->setKeyboardInteractivity(
        LayerShellQt::Window::KeyboardInteractivityNone);
    // Anchor all 4 edges → surface fills entire output
    lsWindow->setAnchors(LayerShellQt::Window::Anchors(
        LayerShellQt::Window::AnchorTop |
        LayerShellQt::Window::AnchorBottom |
        LayerShellQt::Window::AnchorLeft |
        LayerShellQt::Window::AnchorRight));
    lsWindow->setExclusiveZone(0);
    lsWindow->setScope(QStringLiteral("osk"));

    controller->setWindow(window);
    controller->setLayerWindow(lsWindow);

    window->show();

    // System tray
    auto *tray = new KStatusNotifierItem(QStringLiteral("osk"), &app);
    tray->setIconByName(QStringLiteral("input-keyboard"));
    tray->setTitle(QStringLiteral("On-Screen Keyboard"));
    tray->setCategory(KStatusNotifierItem::SystemServices);
    tray->setStatus(KStatusNotifierItem::Active);
    tray->setToolTipTitle(QStringLiteral("On-Screen Keyboard"));
    tray->setToolTipSubTitle(QStringLiteral("Click to toggle"));

    QObject::connect(tray, &KStatusNotifierItem::activateRequested,
                     window, [window](bool, const QPoint &) {
        window->setVisible(!window->isVisible());
    });

    // Global shortcut: Meta+K
    auto *toggleAction = new QAction(QStringLiteral("Toggle On-Screen Keyboard"), &app);
    toggleAction->setObjectName(QStringLiteral("toggle-osk"));
    KGlobalAccel::self()->setDefaultShortcut(toggleAction,
        {QKeySequence(Qt::META | Qt::Key_K)});
    KGlobalAccel::self()->setShortcut(toggleAction,
        {QKeySequence(Qt::META | Qt::Key_K)});
    QObject::connect(toggleAction, &QAction::triggered,
                     window, [window]() {
        window->setVisible(!window->isVisible());
    });

    return app.exec();
}
