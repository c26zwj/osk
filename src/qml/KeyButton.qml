import QtQuick
import QtQuick.Controls

AbstractButton {
    id: root
    focusPolicy: Qt.NoFocus

    property string label: ""
    property string shiftLabel: ""
    property int keyCode: -1
    property real keyWidth: 1.0
    property bool isModifier: false
    property bool modifierActive: false
    property bool modifierLocked: false
    property string flashText: ""

    implicitWidth: Theme.keyHeight * keyWidth + (keyWidth > 1 ? Theme.keySpacing * (keyWidth - 1) : 0)
    implicitHeight: Theme.keyHeight

    readonly property bool _isLetter: label.length === 1 && label >= "a" && label <= "z"

    readonly property string displayLabel: {
        if (shiftLabel !== "" && (KeyboardController.shiftActive || KeyboardController.capsLockActive))
            return shiftLabel;
        return label;
    }

    Timer {
        id: flashTimer
        interval: 300
        onTriggered: root.flashText = ""
    }

    background: Rectangle {
        radius: Theme.keyRadius
        color: {
            if (root.pressed || rightClickArea.pressed) return Theme.keyBackgroundPressed;
            if (root.isModifier && root.modifierLocked)
                return "#c0392b";
            if (root.isModifier && root.modifierActive)
                return Theme.keyBackgroundModActive;
            return Theme.keyBackground;
        }
        border.width: KeyboardController.keyBorderEnabled ? 1 : 0
        border.color: Theme.keyTextDim

        Behavior on color { ColorAnimation { duration: 80 } }
    }

    contentItem: Item {
        // Flash overlay (shown briefly on right-click)
        Text {
            visible: root.flashText !== ""
            anchors.centerIn: parent
            text: root.flashText
            color: Theme.keyBackgroundPressed
            font.pixelSize: Theme.fontSize + 2
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        // Dual label: shift symbol small on top, main below (non-letter keys with shiftLabel)
        Column {
            visible: root.flashText === "" && root.shiftLabel !== "" && !root._isLetter
            anchors.centerIn: parent
            spacing: 0

            Text {
                text: root.shiftLabel
                color: Theme.keyTextDim
                font.pixelSize: 9
                horizontalAlignment: Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Text {
                text: root.label
                color: Theme.keyText
                font.pixelSize: Theme.fontSize
                horizontalAlignment: Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        // Single label: letters, modifiers, special keys
        Text {
            visible: root.flashText === "" && (root.shiftLabel === "" || root._isLetter)
            anchors.centerIn: parent
            text: root.displayLabel
            color: Theme.keyText
            font.pixelSize: root.label.length > 3 ? Theme.smallFontSize : Theme.fontSize
            font.bold: root.isModifier && root.modifierActive
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    // Key repeat: fires initial key on press, then repeats after delay
    Timer {
        id: repeatDelay
        interval: KeyboardController.keyRepeatDelay
        repeat: false
        onTriggered: repeatTimer.start()
    }

    Timer {
        id: repeatTimer
        interval: KeyboardController.keyRepeatInterval
        repeat: true
        onTriggered: {
            if (!root.isModifier && root.keyCode >= 0)
                KeyboardController.pressKey(root.keyCode);
        }
    }

    onPressed: {
        if (!isModifier && keyCode >= 0) {
            KeyboardController.pressKey(keyCode);
            repeatDelay.start();
        }
    }

    onReleased: {
        repeatDelay.stop();
        repeatTimer.stop();
    }

    onCanceled: {
        repeatDelay.stop();
        repeatTimer.stop();
    }

    // Right-click sends shift variant with highlight and flash
    MouseArea {
        id: rightClickArea
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onPressed: {
            if (!root.isModifier && root.keyCode >= 0) {
                root.flashText = root.shiftLabel !== "" ? root.shiftLabel : root.label;
                flashTimer.restart();
                if (!KeyboardController.shiftActive)
                    KeyboardController.toggleShift();
                KeyboardController.pressKey(root.keyCode);
            }
        }
    }
}
