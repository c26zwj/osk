import QtQuick

Column {
    spacing: Theme.keySpacing

    // Row 1: +, /, *, -
    Row {
        spacing: Theme.keySpacing
        KeyButton { label: "+"; keyCode: 78 }
        KeyButton { label: "/"; keyCode: 98 }
        KeyButton { label: "*"; keyCode: 55 }
        KeyButton { label: "-"; keyCode: 74 }
    }

    // Row 2: 7, 8, 9, Del/Bksp
    Row {
        spacing: Theme.keySpacing
        KeyButton { label: "7"; keyCode: 71 }
        KeyButton { label: "8"; keyCode: 72 }
        KeyButton { label: "9"; keyCode: 73 }
        KeyButton {
            label: KeyboardController.compactMode ? "Del" : "Bksp"
            keyCode: KeyboardController.compactMode ? 111 : 14
        }
    }

    // Rows 3+4: 4-6 and 1-3 beside a tall Enter key
    Row {
        spacing: Theme.keySpacing
        Column {
            spacing: Theme.keySpacing
            Row {
                spacing: Theme.keySpacing
                KeyButton { label: "4"; keyCode: 75 }
                KeyButton { label: "5"; keyCode: 76 }
                KeyButton { label: "6"; keyCode: 77 }
            }
            Row {
                spacing: Theme.keySpacing
                KeyButton { label: "1"; keyCode: 79 }
                KeyButton { label: "2"; keyCode: 80 }
                KeyButton { label: "3"; keyCode: 81 }
            }
        }
        KeyButton {
            label: "\u23CE"
            keyCode: 96
            height: Theme.keyHeight * 2 + Theme.keySpacing
        }
    }

    // Row 5: 0 (wide), .
    Row {
        spacing: Theme.keySpacing
        KeyButton { label: "0"; keyCode: 82; keyWidth: 2.0 }
        KeyButton { label: "."; keyCode: 83 }
    }
}
