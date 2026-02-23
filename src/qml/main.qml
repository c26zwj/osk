import QtQuick
import QtQuick.Window

Window {
    id: rootWindow
    visible: true
    color: "transparent"

    property bool anyPageVisible: KeyboardController.shortcutPageVisible
                                  || KeyboardController.clipboardPageVisible
                                  || KeyboardController.settingsVisible
    property int pagePanelHeight: KeyboardController.pagePanelHeight

    function updateRegion() {
        var ry = keyboardPanel.y
        var rh = keyboardPanel.height
        if (anyPageVisible) {
            ry = Math.min(keyboardPanel.y, pagePanel.y)
            var bottom = Math.max(keyboardPanel.y + keyboardPanel.height,
                                  pagePanel.y + pagePanel.height)
            rh = bottom - ry
        }
        KeyboardController.updateInputRegion(keyboardPanel.x, ry, keyboardPanel.width, rh)
    }

    onAnyPageVisibleChanged: updateRegion()

    // Whether the page panel should appear below the keyboard (sticky top)
    property bool panelBelow: KeyboardController.stickyPosition === 1

    // Extension panel above (or below when sticky top) the keyboard
    Rectangle {
        id: pagePanel
        visible: anyPageVisible
        x: keyboardPanel.x
        y: panelBelow ? keyboardPanel.y + keyboardPanel.height
                      : keyboardPanel.y - pagePanelHeight
        width: keyboardPanel.width
        height: pagePanelHeight
        color: Theme.keyboardBackground
        opacity: KeyboardController.opacity
        radius: 6

        // Cover corners so it connects seamlessly to keyboard
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: panelBelow ? parent.top : undefined
            anchors.bottom: panelBelow ? undefined : parent.bottom
            height: 6
            color: parent.color
        }

        // Resize handle (at the edge away from keyboard)
        Rectangle {
            anchors.top: panelBelow ? undefined : parent.top
            anchors.bottom: panelBelow ? parent.bottom : undefined
            anchors.left: parent.left
            anchors.right: parent.right
            height: 6
            color: "transparent"

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeVerCursor
                property real pressY
                property real startHeight

                onPressed: (mouse) => {
                    var scenePos = mapToItem(null, mouse.x, mouse.y)
                    pressY = scenePos.y
                    startHeight = pagePanelHeight
                }
                onPositionChanged: (mouse) => {
                    var scenePos = mapToItem(null, mouse.x, mouse.y)
                    var delta = scenePos.y - pressY
                    var newH = Math.round(startHeight + (panelBelow ? delta : -delta))
                    KeyboardController.setPagePanelHeight(newH)
                }
            }
        }

        // Close button
        Rectangle {
            z: 1
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: 6
            anchors.rightMargin: 6
            width: 20; height: 20; radius: 4
            color: pagePanelCloseMa.containsMouse ? "#c0392b" : Theme.keyBackground
            Text {
                anchors.centerIn: parent
                text: "\u2715"
                color: pagePanelCloseMa.containsMouse ? "#ffffff" : Theme.keyTextDim
                font.pixelSize: 10
            }
            MouseArea {
                id: pagePanelCloseMa; anchors.fill: parent; hoverEnabled: true
                onClicked: {
                    KeyboardController.setShortcutPageVisible(false)
                    KeyboardController.setClipboardPageVisible(false)
                    KeyboardController.setSettingsVisible(false)
                }
            }
        }

        Item {
            anchors.fill: parent
            anchors.topMargin: 6
            clip: true

            ShortcutsPage {
                anchors.fill: parent
            }

            ClipboardPage {
                anchors.fill: parent
            }

            SettingsPage {
                anchors.fill: parent
            }
        }
    }

    // The keyboard panel, positioned freely inside the fullscreen overlay
    Rectangle {
        id: keyboardPanel
        x: KeyboardController.stickyPosition !== 0
           ? Math.round((rootWindow.width - width) / 2)
           : KeyboardController.panelX >= 0 ? KeyboardController.panelX : 200
        y: KeyboardController.stickyPosition === 1 ? 0
         : KeyboardController.stickyPosition === 2 ? Math.max(0, rootWindow.height - height)
         : KeyboardController.panelY >= 0 ? KeyboardController.panelY
            : Math.max(0, (rootWindow.height || 800) - 320)
        // Widen panel proportionally when numpad is visible to prevent key stretching
        readonly property real numpadWidthRatio: KeyboardController.numpadVisible && scaler.kbWidth > 0
            ? (scaler.kbWidth + Theme.keyHeight + Theme.keySpacing + scaler.numpadWidth)
              / (scaler.kbWidth + Theme.keyHeight)
            : 1
        width: {
            var base = KeyboardController.stickyPosition !== 0
                       ? Math.round(rootWindow.width * 2 / 3)
                       : KeyboardController.keyboardWidth
            return Math.round(base * numpadWidthRatio)
        }
        height: {
            var h = KeyboardController.compactMode
                    ? Math.round(KeyboardController.keyboardHeight * 5 / 6)
                    : KeyboardController.keyboardHeight
            if (KeyboardController.stickyPosition !== 0 && KeyboardController.keyboardWidth > 0) {
                var stickyScale = (rootWindow.width * 2 / 3) / KeyboardController.keyboardWidth
                h = Math.round(h * stickyScale)
            }
            return h
        }
        color: Theme.keyboardBackground
        opacity: KeyboardController.opacity
        radius: 6

        // Update input region whenever position or size changes
        onXChanged: updateRegion()
        onYChanged: updateRegion()
        onWidthChanged: updateRegion()
        onHeightChanged: updateRegion()
        Component.onCompleted: updateRegion()

        // Drag bar at top
        Rectangle {
            id: dragBar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 27
            color: Theme.dragBarBackground
            radius: anyPageVisible ? 0 : 6

            // Cover the bottom corners so they don't round into the content
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 4
                color: parent.color
            }

            // Drag dots in center
            Row {
                anchors.centerIn: parent
                spacing: 4
                Repeater {
                    model: 5
                    Rectangle {
                        width: 3; height: 3; radius: 1.5
                        color: Theme.keyTextDim
                    }
                }
            }

            // Drag mouse area
            MouseArea {
                id: dragArea
                anchors.fill: parent
                property point pressPos
                property real panelStartX
                property real panelStartY

                onPressed: (mouse) => {
                    var scenePos = mapToItem(null, mouse.x, mouse.y)
                    pressPos = scenePos
                    panelStartX = keyboardPanel.x
                    panelStartY = keyboardPanel.y
                }
                onPositionChanged: (mouse) => {
                    if (KeyboardController.stickyPosition !== 0) return

                    var scenePos = mapToItem(null, mouse.x, mouse.y)
                    var newX = panelStartX + (scenePos.x - pressPos.x)
                    var panelCenter = newX + keyboardPanel.width / 2

                    if (panelCenter > rootWindow.width && KeyboardController.switchScreen(1)) {
                        newX = 0
                        pressPos = scenePos
                        panelStartX = 0
                    } else if (newX < 0 && KeyboardController.switchScreen(-1)) {
                        newX = rootWindow.width - keyboardPanel.width
                        pressPos = scenePos
                        panelStartX = newX
                    }

                    keyboardPanel.x = Math.max(0, newX)
                    var newY = Math.max(0, panelStartY + (scenePos.y - pressPos.y))
                    keyboardPanel.y = newY
                }
                onReleased: {
                    KeyboardController.savePanelPosition(keyboardPanel.x, keyboardPanel.y)
                }
            }

            // Control buttons (on top of drag area)
            Row {
                z: 1
                anchors.right: parent.right
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4

                // Microphone (compact mode only)
                Rectangle {
                    visible: KeyboardController.compactMode
                    width: 22; height: 22; radius: 4
                    color: voiceBarMa.containsMouse
                           ? (KeyboardController.voiceRecording ? "#c0392b" : Theme.keyBackground)
                           : (KeyboardController.voiceRecording ? "#c0392b" : "transparent")
                    Text {
                        anchors.centerIn: parent
                        text: "\uD83C\uDFA4"
                        color: KeyboardController.voiceRecording ? "#ffffff" : Theme.keyTextDim
                        font.pixelSize: 13
                    }
                    MouseArea {
                        id: voiceBarMa; anchors.fill: parent; hoverEnabled: true
                        onClicked: KeyboardController.toggleVoiceTyping()
                    }
                }

                // Compact mode toggle
                Rectangle {
                    width: 22; height: 22; radius: 4
                    color: compactMa.containsMouse
                           ? (KeyboardController.compactMode ? Theme.keyBackgroundModActive : Theme.keyBackground)
                           : (KeyboardController.compactMode ? Theme.keyBackgroundModActive : "transparent")
                    Text {
                        anchors.centerIn: parent
                        text: "C"
                        color: KeyboardController.compactMode ? Theme.keyText : Theme.keyTextDim
                        font.pixelSize: 11
                        font.bold: true
                    }
                    MouseArea {
                        id: compactMa; anchors.fill: parent; hoverEnabled: true
                        onClicked: KeyboardController.setCompactMode(!KeyboardController.compactMode)
                    }
                }

                // Numpad toggle
                Rectangle {
                    width: 22; height: 22; radius: 4
                    color: numpadMa.containsMouse
                           ? (KeyboardController.numpadVisible ? Theme.keyBackgroundModActive : Theme.keyBackground)
                           : (KeyboardController.numpadVisible ? Theme.keyBackgroundModActive : "transparent")
                    Text {
                        anchors.centerIn: parent
                        text: "#"
                        color: KeyboardController.numpadVisible ? Theme.keyText : Theme.keyTextDim
                        font.pixelSize: 12
                        font.bold: true
                    }
                    MouseArea {
                        id: numpadMa; anchors.fill: parent; hoverEnabled: true
                        onClicked: KeyboardController.setNumpadVisible(!KeyboardController.numpadVisible)
                    }
                }

                // Settings gear
                Rectangle {
                    width: 22; height: 22; radius: 4
                    color: settingsMa.containsMouse
                           ? (KeyboardController.settingsVisible ? Theme.keyBackgroundModActive : Theme.keyBackground)
                           : (KeyboardController.settingsVisible ? Theme.keyBackgroundModActive : "transparent")
                    Text {
                        anchors.centerIn: parent
                        text: "\u2699"
                        color: KeyboardController.settingsVisible ? Theme.keyText : Theme.keyTextDim
                        font.pixelSize: 16
                    }
                    MouseArea {
                        id: settingsMa; anchors.fill: parent; hoverEnabled: true
                        onClicked: {
                            KeyboardController.setShortcutPageVisible(false)
                            KeyboardController.setClipboardPageVisible(false)
                            KeyboardController.setSettingsVisible(!KeyboardController.settingsVisible)
                        }
                    }
                }

                // Minimize to tray
                Rectangle {
                    width: 22; height: 22; radius: 4
                    color: minMa.containsMouse ? Theme.keyBackground : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "\u2013"
                        color: Theme.keyTextDim
                        font.pixelSize: 16
                    }
                    MouseArea {
                        id: minMa; anchors.fill: parent; hoverEnabled: true
                        onClicked: KeyboardController.minimizeToTray()
                    }
                }

                // Close app
                Rectangle {
                    width: 22; height: 22; radius: 4
                    color: closeMa.containsMouse ? "#c0392b" : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "\u2715"
                        color: closeMa.containsMouse ? "#ffffff" : Theme.keyTextDim
                        font.pixelSize: 13
                    }
                    MouseArea {
                        id: closeMa; anchors.fill: parent; hoverEnabled: true
                        onClicked: KeyboardController.closeApp()
                    }
                }
            }
        }

        // Resize handle at bottom-right corner
        Rectangle {
            id: resizeHandle
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: 16; height: 16
            color: "transparent"

            Canvas {
                anchors.fill: parent
                onPaint: {
                    var ctx = getContext("2d");
                    ctx.strokeStyle = "#555";
                    ctx.lineWidth = 1;
                    for (var i = 0; i < 3; i++) {
                        var off = 4 + i * 4;
                        ctx.moveTo(width, off);
                        ctx.lineTo(off, height);
                        ctx.stroke();
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.SizeFDiagCursor
                property point pressPos
                property real startW
                property real startH

                onPressed: (mouse) => {
                    var scenePos = mapToItem(null, mouse.x, mouse.y)
                    pressPos = scenePos
                    startW = KeyboardController.keyboardWidth
                    startH = KeyboardController.keyboardHeight
                }
                onPositionChanged: (mouse) => {
                    var scenePos = mapToItem(null, mouse.x, mouse.y)
                    KeyboardController.setKeyboardWidth(Math.round(startW + (scenePos.x - pressPos.x)))
                    KeyboardController.setKeyboardHeight(Math.round(startH + (scenePos.y - pressPos.y)))
                }
            }
        }

        // Scaled keyboard content
        Item {
            id: contentArea
            anchors.top: dragBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            anchors.bottomMargin: 6
            clip: true

            Item {
                id: scaler
                // Keyboard: 14.5 keys wide + 13 gaps + 2 margins
                readonly property int kbWidth: 14.5 * Theme.keyHeight + 15 * Theme.keySpacing
                // Keyboard rows: 6 normal, 5 compact (no function row)
                readonly property int rowCount: KeyboardController.compactMode ? 5 : 6
                readonly property int kbHeight: rowCount * Theme.keyHeight + (rowCount + 1) * Theme.keySpacing
                // Numpad: 4 keys wide + 3 gaps
                readonly property int numpadWidth: 4 * Theme.keyHeight + 3 * Theme.keySpacing
                width: kbWidth + Theme.keyHeight
                       + (KeyboardController.numpadVisible ? Theme.keySpacing + numpadWidth : 0)
                height: kbHeight
                transformOrigin: Item.TopLeft
                transform: Scale {
                    xScale: scaler.width > 0 ? contentArea.width / scaler.width : 1
                    yScale: scaler.height > 0 ? contentArea.height / scaler.height : 1
                }

                KeyboardLayout {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    width: scaler.kbWidth
                }

                // Right-side button column (inside scaler, uses Theme dimensions)
                Column {
                    id: sideButtons
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    x: scaler.kbWidth
                    width: Theme.keyHeight
                    spacing: Theme.keySpacing
                    topPadding: Theme.keySpacing

                    Repeater {
                        model: [
                            { action: "cut" },
                            { action: "copy" },
                            { action: "paste" },
                            { action: "home" },
                            { action: "end" },
                            { action: "voice" }
                        ]

                        Rectangle {
                            required property var modelData
                            visible: !(KeyboardController.compactMode && modelData.action === "voice")
                            width: Theme.keyHeight
                            height: visible ? Theme.keyHeight : 0
                            radius: Theme.keyRadius
                            color: {
                                if (sideButtonMa.pressed && Theme.keyPressEnabled) return Theme.keyBackgroundPressed;
                                if (modelData.action === "voice" && KeyboardController.voiceRecording)
                                    return "#c0392b";
                                return Theme.keyBackground;
                            }
                            border.width: KeyboardController.keyBorderEnabled ? 1 : 0
                            border.color: Theme.keyBorderColor

                            Canvas {
                                id: iconCanvas
                                anchors.centerIn: parent
                                width: 22; height: 22
                                property color iconColor: (modelData.action === "voice" && KeyboardController.voiceRecording)
                                                        ? "#ffffff" : Theme.keyText
                                onIconColorChanged: requestPaint()
                                onPaint: {
                                    var ctx = getContext("2d");
                                    ctx.reset();
                                    ctx.clearRect(0, 0, width, height);
                                    var c = iconColor.toString();
                                    ctx.strokeStyle = c;
                                    ctx.fillStyle = c;
                                    ctx.lineWidth = 1.8;
                                    ctx.lineCap = "round";
                                    ctx.lineJoin = "round";

                                    if (modelData.action === "cut") {
                                        ctx.beginPath();
                                        ctx.arc(6, 16, 3.5, 0, Math.PI * 2);
                                        ctx.stroke();
                                        ctx.beginPath();
                                        ctx.arc(16, 16, 3.5, 0, Math.PI * 2);
                                        ctx.stroke();
                                        ctx.beginPath();
                                        ctx.moveTo(7.5, 12.5);
                                        ctx.lineTo(14.5, 3);
                                        ctx.stroke();
                                        ctx.beginPath();
                                        ctx.moveTo(14.5, 12.5);
                                        ctx.lineTo(7.5, 3);
                                        ctx.stroke();
                                    } else if (modelData.action === "copy") {
                                        var bg = parent.color.toString();
                                        ctx.lineWidth = 1.6;
                                        ctx.strokeRect(1, 5, 12, 15);
                                        ctx.fillStyle = bg;
                                        ctx.fillRect(7, 1, 12, 15);
                                        ctx.fillStyle = c;
                                        ctx.strokeStyle = c;
                                        ctx.strokeRect(7, 1, 12, 15);
                                    } else if (modelData.action === "paste") {
                                        ctx.lineWidth = 1.6;
                                        ctx.strokeRect(3, 5, 16, 16);
                                        ctx.fillRect(7, 2, 8, 5);
                                        var bg2 = parent.color.toString();
                                        ctx.fillStyle = bg2;
                                        ctx.fillRect(9, 3, 4, 3);
                                        ctx.fillStyle = c;
                                        ctx.beginPath();
                                        ctx.moveTo(7, 12); ctx.lineTo(15, 12);
                                        ctx.stroke();
                                        ctx.beginPath();
                                        ctx.moveTo(7, 16); ctx.lineTo(15, 16);
                                        ctx.stroke();
                                    } else if (modelData.action === "home") {
                                        ctx.lineWidth = 2.2;
                                        ctx.beginPath();
                                        ctx.moveTo(3, 3); ctx.lineTo(3, 19);
                                        ctx.stroke();
                                        ctx.beginPath();
                                        ctx.moveTo(19, 11); ctx.lineTo(8, 11);
                                        ctx.stroke();
                                        ctx.beginPath();
                                        ctx.moveTo(12, 6); ctx.lineTo(8, 11); ctx.lineTo(12, 16);
                                        ctx.stroke();
                                    } else if (modelData.action === "end") {
                                        ctx.lineWidth = 2.2;
                                        ctx.beginPath();
                                        ctx.moveTo(19, 3); ctx.lineTo(19, 19);
                                        ctx.stroke();
                                        ctx.beginPath();
                                        ctx.moveTo(3, 11); ctx.lineTo(14, 11);
                                        ctx.stroke();
                                        ctx.beginPath();
                                        ctx.moveTo(10, 6); ctx.lineTo(14, 11); ctx.lineTo(10, 16);
                                        ctx.stroke();
                                    } else if (modelData.action === "voice") {
                                        ctx.lineWidth = 1.8;
                                        ctx.beginPath();
                                        ctx.moveTo(8, 3); ctx.lineTo(8, 11);
                                        ctx.arc(11, 11, 3, Math.PI, 0, true);
                                        ctx.lineTo(14, 3);
                                        ctx.arc(11, 3, 3, 0, Math.PI, true);
                                        ctx.stroke();
                                        ctx.beginPath();
                                        ctx.moveTo(5, 9);
                                        ctx.quadraticCurveTo(5, 16, 11, 16);
                                        ctx.quadraticCurveTo(17, 16, 17, 9);
                                        ctx.stroke();
                                        ctx.beginPath();
                                        ctx.moveTo(11, 16); ctx.lineTo(11, 20);
                                        ctx.stroke();
                                        ctx.beginPath();
                                        ctx.moveTo(7, 20); ctx.lineTo(15, 20);
                                        ctx.stroke();
                                    }
                                }
                            }

                            MouseArea {
                                id: sideButtonMa
                                anchors.fill: parent
                                onClicked: {
                                    if (modelData.action === "cut") KeyboardController.pressCtrlCombo(45)
                                    else if (modelData.action === "copy") KeyboardController.pressCtrlCombo(46)
                                    else if (modelData.action === "paste") KeyboardController.pressCtrlCombo(47)
                                    else if (modelData.action === "home") KeyboardController.pressKey(102)
                                    else if (modelData.action === "end") KeyboardController.pressKey(107)
                                    else if (modelData.action === "voice") KeyboardController.toggleVoiceTyping()
                                }
                            }
                        }
                    }
                }

                NumpadPage {
                    visible: KeyboardController.numpadVisible
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.topMargin: Theme.keySpacing
                    width: scaler.numpadWidth
                }
            }

            // Size popup overlay
            Rectangle {
                anchors.fill: parent
                visible: KeyboardController.sizePopupVisible
                color: Theme.keyboardBackground

                MouseArea { anchors.fill: parent }

                Column {
                    anchors.centerIn: parent
                    spacing: 10

                    // Header
                    Item {
                        width: 260
                        height: 24

                        Text {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Keyboard Size"
                            color: Theme.keyText
                            font.pixelSize: 16
                            font.bold: true
                        }

                        Rectangle {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            width: 50; height: 24; radius: 4
                            color: sizeCloseMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                            Text { anchors.centerIn: parent; text: "Close"; color: Theme.keyText; font.pixelSize: 11 }
                            MouseArea {
                                id: sizeCloseMa; anchors.fill: parent
                                onClicked: KeyboardController.setSizePopupVisible(false)
                            }
                        }
                    }

                    // Width control
                    Row {
                        spacing: 8
                        anchors.horizontalCenter: parent.horizontalCenter

                        Text {
                            width: 50
                            text: "Width"
                            color: Theme.keyTextDim
                            font.pixelSize: 12
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Rectangle {
                            width: 32; height: 28; radius: 4
                            color: wMinusMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                            Text { anchors.centerIn: parent; text: "\u2212"; color: Theme.keyText; font.pixelSize: 14 }
                            MouseArea { id: wMinusMa; anchors.fill: parent; onClicked: KeyboardController.setKeyboardWidth(KeyboardController.keyboardWidth - 50) }
                        }

                        Text {
                            width: 50
                            text: KeyboardController.keyboardWidth
                            color: Theme.keyText
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Rectangle {
                            width: 32; height: 28; radius: 4
                            color: wPlusMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                            Text { anchors.centerIn: parent; text: "+"; color: Theme.keyText; font.pixelSize: 14 }
                            MouseArea { id: wPlusMa; anchors.fill: parent; onClicked: KeyboardController.setKeyboardWidth(KeyboardController.keyboardWidth + 50) }
                        }
                    }

                    // Height control
                    Row {
                        spacing: 8
                        anchors.horizontalCenter: parent.horizontalCenter

                        Text {
                            width: 50
                            text: "Height"
                            color: Theme.keyTextDim
                            font.pixelSize: 12
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Rectangle {
                            width: 32; height: 28; radius: 4
                            color: hMinusMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                            Text { anchors.centerIn: parent; text: "\u2212"; color: Theme.keyText; font.pixelSize: 14 }
                            MouseArea { id: hMinusMa; anchors.fill: parent; onClicked: KeyboardController.setKeyboardHeight(KeyboardController.keyboardHeight - 25) }
                        }

                        Text {
                            width: 50
                            text: KeyboardController.keyboardHeight
                            color: Theme.keyText
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Rectangle {
                            width: 32; height: 28; radius: 4
                            color: hPlusMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                            Text { anchors.centerIn: parent; text: "+"; color: Theme.keyText; font.pixelSize: 14 }
                            MouseArea { id: hPlusMa; anchors.fill: parent; onClicked: KeyboardController.setKeyboardHeight(KeyboardController.keyboardHeight + 25) }
                        }
                    }
                }
            }
        }
    }
}
