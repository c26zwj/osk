import QtQuick
import QtQuick.Window

Window {
    id: rootWindow
    visible: true
    color: "transparent"

    property bool anyPageVisible: KeyboardController.shortcutPageVisible
                                  || KeyboardController.clipboardPageVisible
                                  || KeyboardController.settingsVisible
    property int pagePanelHeight: 250

    function updateRegion() {
        var ry = keyboardPanel.y
        var rh = keyboardPanel.height
        if (anyPageVisible) {
            ry = pagePanel.y
            rh = keyboardPanel.y + keyboardPanel.height - pagePanel.y
        }
        KeyboardController.updateInputRegion(keyboardPanel.x, ry, keyboardPanel.width, rh)
    }

    onAnyPageVisibleChanged: updateRegion()

    // Extension panel above the keyboard for shortcuts/clipboard/settings
    Rectangle {
        id: pagePanel
        visible: anyPageVisible
        x: keyboardPanel.x
        y: keyboardPanel.y - pagePanelHeight
        width: keyboardPanel.width
        height: pagePanelHeight
        color: Theme.keyboardBackground
        radius: 6

        // Cover bottom corners so it connects seamlessly to keyboard below
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 6
            color: parent.color
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
        x: KeyboardController.panelX >= 0 ? KeyboardController.panelX : 200
        y: KeyboardController.panelY >= 0 ? KeyboardController.panelY : Math.max(0, (rootWindow.height || 800) - 320)
        width: KeyboardController.keyboardWidth
        height: KeyboardController.keyboardHeight
        color: Theme.keyboardBackground
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
            height: 20
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
                    var scenePos = mapToItem(null, mouse.x, mouse.y)
                    var newX = panelStartX + (scenePos.x - pressPos.x)
                    var newY = Math.max(0, panelStartY + (scenePos.y - pressPos.y))
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
                    keyboardPanel.y = newY
                }
                onReleased: KeyboardController.savePanelPosition(keyboardPanel.x, keyboardPanel.y)
            }

            // Control buttons (on top of drag area)
            Row {
                z: 1
                anchors.right: parent.right
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                spacing: 3

                // Minimize to tray
                Rectangle {
                    width: 16; height: 16; radius: 3
                    color: minMa.containsMouse ? Theme.keyBackground : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "\u2013"
                        color: Theme.keyTextDim
                        font.pixelSize: 12
                    }
                    MouseArea {
                        id: minMa; anchors.fill: parent; hoverEnabled: true
                        onClicked: KeyboardController.minimizeToTray()
                    }
                }

                // Close app
                Rectangle {
                    width: 16; height: 16; radius: 3
                    color: closeMa.containsMouse ? "#c0392b" : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "\u2715"
                        color: closeMa.containsMouse ? "#ffffff" : Theme.keyTextDim
                        font.pixelSize: 10
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

        // Right-side button column (aligned with keyboard rows)
        Column {
            id: sideButtons
            anchors.top: dragBar.bottom
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 3
            anchors.bottomMargin: 6
            width: 36

            // Match keyboard scaler proportions (273 total, 3px margins, 42px keys, 3px gaps)
            property real yScale: height / 273
            topPadding: Math.round(3 * yScale)
            spacing: Math.round(3 * yScale)

            Repeater {
                model: [
                    { label: "\u2702", action: "cut",      iconColor: "#e06666" },
                    { label: "\u29c9", action: "copy",     iconColor: "#6fa3d6" },
                    { label: "\u2398", action: "paste",    iconColor: "#6cc070" },
                    { label: "Hm",     action: "home",     iconColor: "" },
                    { label: "End",    action: "end",      iconColor: "" },
                    { label: "\u2699", action: "settings", iconColor: "" }
                ]

                Rectangle {
                    required property var modelData
                    width: sideButtons.width
                    height: Math.round(42 * sideButtons.yScale)
                    radius: 4
                    color: {
                        if (sideButtonMa.pressed) return Theme.keyBackgroundPressed;
                        if (modelData.action === "settings" && KeyboardController.settingsVisible)
                            return Theme.keyBackgroundModActive;
                        return Theme.keyBackground;
                    }
                    border.width: KeyboardController.keyBorderEnabled ? 1 : 0
                    border.color: Theme.keyTextDim

                    Text {
                        anchors.centerIn: parent
                        text: modelData.label
                        color: modelData.iconColor || Theme.keyText
                        font.pixelSize: modelData.label.length > 2 ? 10 : 16
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
                            else if (modelData.action === "settings") {
                                KeyboardController.setShortcutPageVisible(false)
                                KeyboardController.setClipboardPageVisible(false)
                                KeyboardController.setSettingsVisible(!KeyboardController.settingsVisible)
                            }
                        }
                    }
                }
            }
        }

        // Scaled keyboard content
        Item {
            id: contentArea
            anchors.top: dragBar.bottom
            anchors.left: parent.left
            anchors.right: sideButtons.left
            anchors.bottom: parent.bottom
            anchors.leftMargin: 4
            anchors.rightMargin: 2
            anchors.bottomMargin: 6
            clip: true

            Item {
                id: scaler
                width: 656
                height: 273
                transformOrigin: Item.TopLeft
                transform: Scale {
                    xScale: contentArea.width / scaler.width
                    yScale: contentArea.height / scaler.height
                }

                KeyboardLayout {
                    anchors.fill: parent
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
