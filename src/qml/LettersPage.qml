import QtQuick

Column {
    spacing: Theme.keySpacing

    // Row 0: Esc + Function keys + Del (hidden in compact mode)
    Row {
        visible: !KeyboardController.compactMode
        spacing: Theme.keySpacing
        anchors.horizontalCenter: parent.horizontalCenter

        KeyButton { label: "Esc"; keyCode: 1 }
        KeyButton { label: "F1";  keyCode: 59 }
        KeyButton { label: "F2";  keyCode: 60 }
        KeyButton { label: "F3";  keyCode: 61 }
        KeyButton { label: "F4";  keyCode: 62 }
        KeyButton { label: "F5";  keyCode: 63 }
        KeyButton { label: "F6";  keyCode: 64 }
        KeyButton { label: "F7";  keyCode: 65 }
        KeyButton { label: "F8";  keyCode: 66 }
        KeyButton { label: "F9";  keyCode: 67 }
        KeyButton { label: "F10"; keyCode: 68 }
        KeyButton { label: "F11"; keyCode: 87 }
        KeyButton { label: "F12"; keyCode: 88 }
        KeyButton { label: "Del"; keyCode: 111; keyWidth: 1.5 }
    }

    // Row 1: Number row
    Row {
        spacing: Theme.keySpacing
        anchors.horizontalCenter: parent.horizontalCenter

        KeyButton { label: "`";  shiftLabel: "~"; keyCode: 41 }
        KeyButton { label: "1";  shiftLabel: "!"; keyCode: 2 }
        KeyButton { label: "2";  shiftLabel: "@"; keyCode: 3 }
        KeyButton { label: "3";  shiftLabel: "#"; keyCode: 4 }
        KeyButton { label: "4";  shiftLabel: "$"; keyCode: 5 }
        KeyButton { label: "5";  shiftLabel: "%"; keyCode: 6 }
        KeyButton { label: "6";  shiftLabel: "^"; keyCode: 7 }
        KeyButton { label: "7";  shiftLabel: "&"; keyCode: 8 }
        KeyButton { label: "8";  shiftLabel: "*"; keyCode: 9 }
        KeyButton { label: "9";  shiftLabel: "("; keyCode: 10 }
        KeyButton { label: "0";  shiftLabel: ")"; keyCode: 11 }
        KeyButton { label: "-";  shiftLabel: "_"; keyCode: 12 }
        KeyButton { label: "=";  shiftLabel: "+"; keyCode: 13 }
        KeyButton { label: "\u232b"; keyCode: 14; keyWidth: 1.5 }
    }

    // Row 2: Tab + QWERTY
    Row {
        spacing: Theme.keySpacing
        anchors.horizontalCenter: parent.horizontalCenter

        KeyButton { label: "Tab"; keyCode: 15; keyWidth: 1.5 }
        KeyButton { label: "q"; shiftLabel: "Q"; keyCode: 16 }
        KeyButton { label: "w"; shiftLabel: "W"; keyCode: 17 }
        KeyButton { label: "e"; shiftLabel: "E"; keyCode: 18 }
        KeyButton { label: "r"; shiftLabel: "R"; keyCode: 19 }
        KeyButton { label: "t"; shiftLabel: "T"; keyCode: 20 }
        KeyButton { label: "y"; shiftLabel: "Y"; keyCode: 21 }
        KeyButton { label: "u"; shiftLabel: "U"; keyCode: 22 }
        KeyButton { label: "i"; shiftLabel: "I"; keyCode: 23 }
        KeyButton { label: "o"; shiftLabel: "O"; keyCode: 24 }
        KeyButton { label: "p"; shiftLabel: "P"; keyCode: 25 }
        KeyButton { label: "["; shiftLabel: "{"; keyCode: 26 }
        KeyButton { label: "]"; shiftLabel: "}"; keyCode: 27 }
        KeyButton { label: "\\"; shiftLabel: "|"; keyCode: 43 }
    }

    // Row 3: Caps + ASDF
    Row {
        spacing: Theme.keySpacing
        anchors.horizontalCenter: parent.horizontalCenter

        KeyButton {
            label: "Caps"; keyWidth: 1.75; isModifier: true
            modifierActive: KeyboardController.capsLockActive
            onClicked: KeyboardController.toggleCapsLock()
        }
        KeyButton { label: "a"; shiftLabel: "A"; keyCode: 30 }
        KeyButton { label: "s"; shiftLabel: "S"; keyCode: 31 }
        KeyButton { label: "d"; shiftLabel: "D"; keyCode: 32 }
        KeyButton { label: "f"; shiftLabel: "F"; keyCode: 33 }
        KeyButton { label: "g"; shiftLabel: "G"; keyCode: 34 }
        KeyButton { label: "h"; shiftLabel: "H"; keyCode: 35 }
        KeyButton { label: "j"; shiftLabel: "J"; keyCode: 36 }
        KeyButton { label: "k"; shiftLabel: "K"; keyCode: 37 }
        KeyButton { label: "l"; shiftLabel: "L"; keyCode: 38 }
        KeyButton { label: ";"; shiftLabel: ":"; keyCode: 39 }
        KeyButton { label: "'"; shiftLabel: "\""; keyCode: 40 }
        KeyButton { label: "Enter"; keyCode: 28; keyWidth: 1.75 }
    }

    // Row 4: Shift + ZXCV
    Row {
        spacing: Theme.keySpacing
        anchors.horizontalCenter: parent.horizontalCenter

        KeyButton {
            label: "Shift"; keyWidth: 2.25; isModifier: true
            modifierActive: KeyboardController.shiftActive
            modifierLocked: KeyboardController.shiftLocked
            onClicked: KeyboardController.toggleShift()
        }
        KeyButton { label: "z"; shiftLabel: "Z"; keyCode: 44 }
        KeyButton { label: "x"; shiftLabel: "X"; keyCode: 45 }
        KeyButton { label: "c"; shiftLabel: "C"; keyCode: 46 }
        KeyButton { label: "v"; shiftLabel: "V"; keyCode: 47 }
        KeyButton { label: "b"; shiftLabel: "B"; keyCode: 48 }
        KeyButton { label: "n"; shiftLabel: "N"; keyCode: 49 }
        KeyButton { label: "m"; shiftLabel: "M"; keyCode: 50 }
        KeyButton { label: ","; shiftLabel: "<"; keyCode: 51 }
        KeyButton { label: "."; shiftLabel: ">"; keyCode: 52 }
        KeyButton { label: "/"; shiftLabel: "?"; keyCode: 53 }
        KeyButton {
            label: "Shift"; keyWidth: 2.25; isModifier: true
            modifierActive: KeyboardController.shiftActive
            modifierLocked: KeyboardController.shiftLocked
            onClicked: KeyboardController.toggleShift()
        }
    }

    // Row 5: Bottom row
    Row {
        spacing: Theme.keySpacing
        anchors.horizontalCenter: parent.horizontalCenter

        KeyButton {
            label: "Ctrl"; keyWidth: 1.0; isModifier: true
            modifierActive: KeyboardController.ctrlActive
            modifierLocked: KeyboardController.ctrlLocked
            onClicked: KeyboardController.toggleCtrl()
        }
        KeyButton {
            label: "Meta"; keyWidth: 1.0; isModifier: true
            modifierActive: KeyboardController.superActive
            modifierLocked: KeyboardController.superLocked
            onClicked: KeyboardController.toggleSuper()
        }
        KeyButton {
            label: "Alt"; keyWidth: 1.0; isModifier: true
            modifierActive: KeyboardController.altActive
            modifierLocked: KeyboardController.altLocked
            onClicked: KeyboardController.toggleAlt()
        }
        KeyButton { label: ""; keyCode: 57; keyWidth: 4.5 }
        KeyButton {
            label: "Alt"; keyWidth: 1.0; isModifier: true
            modifierActive: KeyboardController.altActive
            modifierLocked: KeyboardController.altLocked
            onClicked: KeyboardController.toggleAlt()
        }
        KeyButton {
            label: "\u00a7"; keyWidth: 1.0; isModifier: true
            modifierActive: KeyboardController.shortcutPageVisible
            onClicked: {
                if (KeyboardController.shortcutPageVisible) {
                    KeyboardController.setShortcutPageVisible(false)
                } else {
                    KeyboardController.setClipboardPageVisible(false)
                    KeyboardController.setSettingsVisible(false)
                    KeyboardController.setShortcutPageVisible(true)
                }
            }
        }
        KeyButton {
            label: "\uD83D\uDCCB"; keyWidth: 1.0; isModifier: true
            modifierActive: KeyboardController.clipboardPageVisible
            onClicked: {
                if (KeyboardController.clipboardPageVisible) {
                    KeyboardController.setClipboardPageVisible(false)
                } else {
                    KeyboardController.setShortcutPageVisible(false)
                    KeyboardController.setSettingsVisible(false)
                    KeyboardController.refreshClipboardHistory()
                    KeyboardController.setClipboardPageVisible(true)
                }
            }
        }
        KeyButton { label: "\u2190"; keyCode: 105 }
        KeyButton { label: "\u2193"; keyCode: 108 }
        KeyButton { label: "\u2191"; keyCode: 103 }
        KeyButton { label: "\u2192"; keyCode: 106 }
    }
}
