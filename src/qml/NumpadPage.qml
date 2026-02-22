import QtQuick

Column {
    spacing: Theme.keySpacing

    // Row 1: NumLock, /, *, -
    Row {
        spacing: Theme.keySpacing
        KeyButton { label: "Num"; keyCode: 69 }
        KeyButton { label: "/"; keyCode: 98 }
        KeyButton { label: "*"; keyCode: 55 }
        KeyButton { label: "-"; keyCode: 74 }
    }

    // Row 2: 7, 8, 9, +
    Row {
        spacing: Theme.keySpacing
        KeyButton { label: "7"; keyCode: 71 }
        KeyButton { label: "8"; keyCode: 72 }
        KeyButton { label: "9"; keyCode: 73 }
        KeyButton { label: "+"; keyCode: 78 }
    }

    // Row 3: 4, 5, 6
    Row {
        spacing: Theme.keySpacing
        KeyButton { label: "4"; keyCode: 75 }
        KeyButton { label: "5"; keyCode: 76 }
        KeyButton { label: "6"; keyCode: 77 }
    }

    // Row 4: 1, 2, 3, Enter
    Row {
        spacing: Theme.keySpacing
        KeyButton { label: "1"; keyCode: 79 }
        KeyButton { label: "2"; keyCode: 80 }
        KeyButton { label: "3"; keyCode: 81 }
        KeyButton { label: "Ent"; keyCode: 96 }
    }

    // Row 5: 0 (wide), .
    Row {
        spacing: Theme.keySpacing
        KeyButton { label: "0"; keyCode: 82; keyWidth: 2.0 }
        KeyButton { label: "."; keyCode: 83 }
    }
}
