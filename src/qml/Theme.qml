pragma Singleton
import QtQuick

QtObject {
    // Dimensions — bound to controller settings
    readonly property int keyHeight: 42
    readonly property int keySpacing: KeyboardController.keySpacing
    readonly property int keyRadius: KeyboardController.keyRadius
    readonly property int fontSize: KeyboardController.fontSize
    readonly property int smallFontSize: Math.max(8, fontSize - 3)

    // Key colors — derived from background
    readonly property color keyBackground: Qt.lighter(keyboardBackground, 1.5)
    readonly property color keyBackgroundPressed: {
        var c = KeyboardController.keyPressColor;
        return (c && c !== "none") ? c : "#3daee9";
    }
    readonly property color keyBackgroundModActive: "#2980b9"
    readonly property color keyBackgroundLocked: {
        var c = KeyboardController.lockedKeyColor;
        return (c && c !== "none") ? c : "#c0392b";
    }
    readonly property color keyText: "#eff0f1"
    readonly property color keyTextDim: "#7f8c8d"
    readonly property color keyBorderColor: KeyboardController.keyBorderColor || keyTextDim

    // Whether custom press/locked colors are disabled (empty = use default)
    readonly property bool keyPressEnabled: KeyboardController.keyPressColor !== "none"
    readonly property bool lockedKeyEnabled: KeyboardController.lockedKeyColor !== "none"

    // Panel background — user-configurable
    readonly property color keyboardBackground: KeyboardController.backgroundColor
    readonly property color dragBarBackground: Qt.darker(keyboardBackground, 1.25)
}
