pragma Singleton
import QtQuick

QtObject {
    // Dimensions
    readonly property int keyHeight: 42
    readonly property int keySpacing: 3
    readonly property int keyRadius: 6
    readonly property int fontSize: 14
    readonly property int smallFontSize: 11

    // Key colors — derived from background
    readonly property color keyBackground: Qt.lighter(keyboardBackground, 1.5)
    readonly property color keyBackgroundPressed: "#3daee9"
    readonly property color keyBackgroundModActive: "#2980b9"
    readonly property color keyText: "#eff0f1"
    readonly property color keyTextDim: "#7f8c8d"

    // Panel background — user-configurable
    readonly property color keyboardBackground: KeyboardController.backgroundColor
    readonly property color dragBarBackground: Qt.darker(keyboardBackground, 1.25)
}
