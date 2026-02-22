import QtQuick

Rectangle {
    id: settingsRoot
    visible: KeyboardController.settingsVisible
    color: Theme.keyboardBackground

    // Block clicks from reaching the keyboard behind
    MouseArea { anchors.fill: parent }

    // Determine column count based on available width
    readonly property int columnCount: width >= 700 ? 3 : width >= 460 ? 2 : 1
    readonly property real colWidth: (width - 20 - (columnCount - 1) * 10) / columnCount

    Flickable {
        anchors.fill: parent
        anchors.margins: 10
        contentHeight: settingsFlow.height
        clip: true
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.StopAtBounds

        // Title
        Text {
            id: titleText
            text: "Settings"
            color: Theme.keyText
            font.pixelSize: 18
            font.bold: true
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Flow {
            id: settingsFlow
            anchors.top: titleText.bottom
            anchors.topMargin: 10
            width: parent.width
            spacing: 10

            // ============================================================
            // APPEARANCE column
            // ============================================================
            Column {
                width: settingsRoot.colWidth
                spacing: 10

                Text {
                    text: "APPEARANCE"
                    color: Theme.keyTextDim
                    font.pixelSize: 10
                    font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                // Background color palette
                Column {
                    spacing: 6
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Background color:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Grid {
                        columns: 8
                        spacing: 4
                        anchors.horizontalCenter: parent.horizontalCenter

                        Repeater {
                            model: [
                                "#232629", "#2e3440", "#1e1e2e", "#282c34",
                                "#1a1a2e", "#1b2838", "#2d2d2d", "#1e1e1e",
                                "#3c3836", "#292d3e", "#212733", "#1f1f28",
                                "#24273a", "#161616", "#2b2b2b", "#1d2021"
                            ]

                            delegate: Rectangle {
                                required property string modelData
                                width: 28; height: 28; radius: 4
                                color: modelData
                                border.width: KeyboardController.backgroundColor === modelData ? 2 : 0
                                border.color: Theme.keyBackgroundPressed

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: KeyboardController.setBackgroundColor(modelData)
                                }
                            }
                        }
                    }
                }

                // Key press color
                Column {
                    spacing: 4
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Key press color:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Row {
                        spacing: 4
                        anchors.horizontalCenter: parent.horizontalCenter

                        Rectangle {
                            width: 28; height: 28; radius: 4
                            color: Theme.keyBackground
                            border.width: KeyboardController.keyPressColor === "none" ? 2 : 0
                            border.color: Theme.keyBackgroundModActive

                            Text {
                                anchors.centerIn: parent
                                text: "\u2205"
                                color: Theme.keyTextDim
                                font.pixelSize: 14
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: KeyboardController.setKeyPressColor("none")
                            }
                        }

                        Repeater {
                            model: [
                                "#3daee9", "#2980b9", "#e74c3c", "#e67e22",
                                "#27ae60", "#8e44ad", "#f39c12", "#1abc9c"
                            ]

                            Rectangle {
                                required property string modelData
                                width: 28; height: 28; radius: 4
                                color: modelData
                                border.width: KeyboardController.keyPressColor === modelData ? 2 : 0
                                border.color: Theme.keyText

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: KeyboardController.setKeyPressColor(modelData)
                                }
                            }
                        }

                        Rectangle {
                            width: 28; height: 28; radius: 4
                            color: "#3daee9"
                            border.width: KeyboardController.keyPressColor === "" ? 2 : 0
                            border.color: Theme.keyText

                            Text {
                                anchors.centerIn: parent
                                text: "D"
                                color: "#ffffff"
                                font.pixelSize: 11
                                font.bold: true
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: KeyboardController.setKeyPressColor("")
                            }
                        }
                    }
                }

                // Locked key color
                Column {
                    spacing: 4
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Locked key color:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Row {
                        spacing: 4
                        anchors.horizontalCenter: parent.horizontalCenter

                        Rectangle {
                            width: 28; height: 28; radius: 4
                            color: Theme.keyBackground
                            border.width: KeyboardController.lockedKeyColor === "none" ? 2 : 0
                            border.color: Theme.keyBackgroundModActive

                            Text {
                                anchors.centerIn: parent
                                text: "\u2205"
                                color: Theme.keyTextDim
                                font.pixelSize: 14
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: KeyboardController.setLockedKeyColor("none")
                            }
                        }

                        Repeater {
                            model: [
                                "#c0392b", "#e74c3c", "#d35400", "#e67e22",
                                "#8e44ad", "#2980b9", "#27ae60", "#f39c12"
                            ]

                            Rectangle {
                                required property string modelData
                                width: 28; height: 28; radius: 4
                                color: modelData
                                border.width: KeyboardController.lockedKeyColor === modelData ? 2 : 0
                                border.color: Theme.keyText

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: KeyboardController.setLockedKeyColor(modelData)
                                }
                            }
                        }

                        Rectangle {
                            width: 28; height: 28; radius: 4
                            color: "#c0392b"
                            border.width: KeyboardController.lockedKeyColor === "" ? 2 : 0
                            border.color: Theme.keyText

                            Text {
                                anchors.centerIn: parent
                                text: "D"
                                color: "#ffffff"
                                font.pixelSize: 11
                                font.bold: true
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: KeyboardController.setLockedKeyColor("")
                            }
                        }
                    }
                }

                // Key border color
                Column {
                    spacing: 4
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Border color:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Row {
                        spacing: 4
                        anchors.horizontalCenter: parent.horizontalCenter

                        Item { width: 28; height: 28 }

                        Repeater {
                            model: [
                                "#7f8c8d", "#2980b9",
                                "#e74c3c", "#27ae60", "#f39c12", "#8e44ad",
                                "#eff0f1", "#555555"
                            ]

                            Rectangle {
                                required property string modelData
                                width: 28; height: 28; radius: 4
                                color: modelData
                                border.width: KeyboardController.keyBorderColor === modelData ? 2 : 0
                                border.color: Theme.keyText

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: KeyboardController.setKeyBorderColor(modelData)
                                }
                            }
                        }

                        Rectangle {
                            width: 28; height: 28; radius: 4
                            color: "#7f8c8d"
                            border.width: KeyboardController.keyBorderColor === "" ? 2 : 0
                            border.color: Theme.keyText

                            Text {
                                anchors.centerIn: parent
                                text: "D"
                                color: "#ffffff"
                                font.pixelSize: 11
                                font.bold: true
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: KeyboardController.setKeyBorderColor("")
                            }
                        }
                    }
                }

                // Opacity
                Row {
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Opacity:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 100
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 28; height: 28; radius: 4
                        color: decOpacityMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                        Text { anchors.centerIn: parent; text: "-"; color: Theme.keyText; font.pixelSize: 14 }
                        MouseArea {
                            id: decOpacityMa; anchors.fill: parent
                            onClicked: KeyboardController.setOpacity(KeyboardController.opacity - 0.05)
                        }
                    }

                    Text {
                        text: Math.round(KeyboardController.opacity * 100) + "%"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 60
                        horizontalAlignment: Text.AlignHCenter
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 28; height: 28; radius: 4
                        color: incOpacityMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                        Text { anchors.centerIn: parent; text: "+"; color: Theme.keyText; font.pixelSize: 14 }
                        MouseArea {
                            id: incOpacityMa; anchors.fill: parent
                            onClicked: KeyboardController.setOpacity(KeyboardController.opacity + 0.05)
                        }
                    }
                }

                // Font size
                Row {
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Font size:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 100
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 28; height: 28; radius: 4
                        color: decFontMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                        Text { anchors.centerIn: parent; text: "-"; color: Theme.keyText; font.pixelSize: 14 }
                        MouseArea {
                            id: decFontMa; anchors.fill: parent
                            onClicked: KeyboardController.setFontSize(KeyboardController.fontSize - 1)
                        }
                    }

                    Text {
                        text: KeyboardController.fontSize + " px"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 60
                        horizontalAlignment: Text.AlignHCenter
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 28; height: 28; radius: 4
                        color: incFontMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                        Text { anchors.centerIn: parent; text: "+"; color: Theme.keyText; font.pixelSize: 14 }
                        MouseArea {
                            id: incFontMa; anchors.fill: parent
                            onClicked: KeyboardController.setFontSize(KeyboardController.fontSize + 1)
                        }
                    }
                }

                // Corner radius
                Row {
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Corner radius:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 100
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 28; height: 28; radius: 4
                        color: decRadiusMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                        Text { anchors.centerIn: parent; text: "-"; color: Theme.keyText; font.pixelSize: 14 }
                        MouseArea {
                            id: decRadiusMa; anchors.fill: parent
                            onClicked: KeyboardController.setKeyRadius(KeyboardController.keyRadius - 1)
                        }
                    }

                    Text {
                        text: KeyboardController.keyRadius + " px"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 60
                        horizontalAlignment: Text.AlignHCenter
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 28; height: 28; radius: 4
                        color: incRadiusMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                        Text { anchors.centerIn: parent; text: "+"; color: Theme.keyText; font.pixelSize: 14 }
                        MouseArea {
                            id: incRadiusMa; anchors.fill: parent
                            onClicked: KeyboardController.setKeyRadius(KeyboardController.keyRadius + 1)
                        }
                    }
                }

                // Key spacing
                Row {
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Key spacing:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 100
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 28; height: 28; radius: 4
                        color: decSpacingMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                        Text { anchors.centerIn: parent; text: "-"; color: Theme.keyText; font.pixelSize: 14 }
                        MouseArea {
                            id: decSpacingMa; anchors.fill: parent
                            onClicked: KeyboardController.setKeySpacing(KeyboardController.keySpacing - 1)
                        }
                    }

                    Text {
                        text: KeyboardController.keySpacing + " px"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 60
                        horizontalAlignment: Text.AlignHCenter
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 28; height: 28; radius: 4
                        color: incSpacingMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                        Text { anchors.centerIn: parent; text: "+"; color: Theme.keyText; font.pixelSize: 14 }
                        MouseArea {
                            id: incSpacingMa; anchors.fill: parent
                            onClicked: KeyboardController.setKeySpacing(KeyboardController.keySpacing + 1)
                        }
                    }
                }

                // Key border toggle
                Row {
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Key borders:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 100
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 60; height: 28; radius: 4
                        color: KeyboardController.keyBorderEnabled
                               ? Theme.keyBackgroundModActive
                               : Theme.keyBackground

                        Text {
                            anchors.centerIn: parent
                            text: KeyboardController.keyBorderEnabled ? "On" : "Off"
                            color: Theme.keyText
                            font.pixelSize: 13
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: KeyboardController.setKeyBorderEnabled(!KeyboardController.keyBorderEnabled)
                        }
                    }
                }

                Item { width: 1; height: 6 }
            }

            // ============================================================
            // BEHAVIOR column
            // ============================================================
            Column {
                width: settingsRoot.colWidth
                spacing: 10

                Text {
                    text: "BEHAVIOR"
                    color: Theme.keyTextDim
                    font.pixelSize: 10
                    font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                // Key repeat delay
                Row {
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Repeat delay:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 100
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 28; height: 28; radius: 4
                        color: decDelayMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                        Text { anchors.centerIn: parent; text: "-"; color: Theme.keyText; font.pixelSize: 14 }
                        MouseArea {
                            id: decDelayMa; anchors.fill: parent
                            onClicked: KeyboardController.setKeyRepeatDelay(KeyboardController.keyRepeatDelay - 50)
                        }
                    }

                    Text {
                        text: KeyboardController.keyRepeatDelay + " ms"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 60
                        horizontalAlignment: Text.AlignHCenter
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 28; height: 28; radius: 4
                        color: incDelayMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                        Text { anchors.centerIn: parent; text: "+"; color: Theme.keyText; font.pixelSize: 14 }
                        MouseArea {
                            id: incDelayMa; anchors.fill: parent
                            onClicked: KeyboardController.setKeyRepeatDelay(KeyboardController.keyRepeatDelay + 50)
                        }
                    }
                }

                // Key repeat interval
                Row {
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Repeat rate:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 100
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 28; height: 28; radius: 4
                        color: decIntMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                        Text { anchors.centerIn: parent; text: "-"; color: Theme.keyText; font.pixelSize: 14 }
                        MouseArea {
                            id: decIntMa; anchors.fill: parent
                            onClicked: KeyboardController.setKeyRepeatInterval(KeyboardController.keyRepeatInterval - 10)
                        }
                    }

                    Text {
                        text: KeyboardController.keyRepeatInterval + " ms"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 60
                        horizontalAlignment: Text.AlignHCenter
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 28; height: 28; radius: 4
                        color: incIntMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                        Text { anchors.centerIn: parent; text: "+"; color: Theme.keyText; font.pixelSize: 14 }
                        MouseArea {
                            id: incIntMa; anchors.fill: parent
                            onClicked: KeyboardController.setKeyRepeatInterval(KeyboardController.keyRepeatInterval + 10)
                        }
                    }
                }

                // Auto-hide delay
                Row {
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Auto-hide:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 100
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 28; height: 28; radius: 4
                        color: decHideMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                        Text { anchors.centerIn: parent; text: "-"; color: Theme.keyText; font.pixelSize: 14 }
                        MouseArea {
                            id: decHideMa; anchors.fill: parent
                            onClicked: KeyboardController.setAutoHideDelay(KeyboardController.autoHideDelay - 5)
                        }
                    }

                    Text {
                        text: KeyboardController.autoHideDelay === 0 ? "Off" : KeyboardController.autoHideDelay + " s"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 60
                        horizontalAlignment: Text.AlignHCenter
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 28; height: 28; radius: 4
                        color: incHideMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                        Text { anchors.centerIn: parent; text: "+"; color: Theme.keyText; font.pixelSize: 14 }
                        MouseArea {
                            id: incHideMa; anchors.fill: parent
                            onClicked: KeyboardController.setAutoHideDelay(KeyboardController.autoHideDelay + 5)
                        }
                    }
                }

                // Sound feedback
                Row {
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Sound feedback:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 120
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 60; height: 28; radius: 4
                        color: KeyboardController.soundFeedback
                               ? Theme.keyBackgroundModActive
                               : Theme.keyBackground

                        Text {
                            anchors.centerIn: parent
                            text: KeyboardController.soundFeedback ? "On" : "Off"
                            color: Theme.keyText
                            font.pixelSize: 13
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: KeyboardController.setSoundFeedback(!KeyboardController.soundFeedback)
                        }
                    }
                }

                // Close on paste
                Row {
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Close on paste:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 120
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 60; height: 28; radius: 4
                        color: KeyboardController.closeOnPaste
                               ? Theme.keyBackgroundModActive
                               : Theme.keyBackground

                        Text {
                            anchors.centerIn: parent
                            text: KeyboardController.closeOnPaste ? "On" : "Off"
                            color: Theme.keyText
                            font.pixelSize: 13
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: KeyboardController.setCloseOnPaste(!KeyboardController.closeOnPaste)
                        }
                    }
                }

                // Close on insert shortcut
                Row {
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Close on shortcut:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 120
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 60; height: 28; radius: 4
                        color: KeyboardController.closeOnInsertShortcut
                               ? Theme.keyBackgroundModActive
                               : Theme.keyBackground

                        Text {
                            anchors.centerIn: parent
                            text: KeyboardController.closeOnInsertShortcut ? "On" : "Off"
                            color: Theme.keyText
                            font.pixelSize: 13
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: KeyboardController.setCloseOnInsertShortcut(!KeyboardController.closeOnInsertShortcut)
                        }
                    }
                }

                // Compact mode
                Row {
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Compact mode:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 120
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 60; height: 28; radius: 4
                        color: KeyboardController.compactMode
                               ? Theme.keyBackgroundModActive
                               : Theme.keyBackground

                        Text {
                            anchors.centerIn: parent
                            text: KeyboardController.compactMode ? "On" : "Off"
                            color: Theme.keyText
                            font.pixelSize: 13
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: KeyboardController.setCompactMode(!KeyboardController.compactMode)
                        }
                    }
                }

                // Numpad
                Row {
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Numpad:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 120
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 60; height: 28; radius: 4
                        color: KeyboardController.numpadVisible
                               ? Theme.keyBackgroundModActive
                               : Theme.keyBackground

                        Text {
                            anchors.centerIn: parent
                            text: KeyboardController.numpadVisible ? "On" : "Off"
                            color: Theme.keyText
                            font.pixelSize: 13
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: KeyboardController.setNumpadVisible(!KeyboardController.numpadVisible)
                        }
                    }
                }

                // Sticky position
                Row {
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Position:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 120
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Row {
                        spacing: 4

                        Repeater {
                            model: [
                                { label: "Float", value: 0 },
                                { label: "Top", value: 1 },
                                { label: "Bottom", value: 2 }
                            ]

                            Rectangle {
                                required property var modelData
                                width: 52; height: 28; radius: 4
                                color: KeyboardController.stickyPosition === modelData.value
                                       ? Theme.keyBackgroundModActive
                                       : Theme.keyBackground

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.label
                                    color: Theme.keyText
                                    font.pixelSize: 12
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: KeyboardController.setStickyPosition(modelData.value)
                                }
                            }
                        }
                    }
                }

                Item { width: 1; height: 6 }
            }

            // ============================================================
            // SYSTEM column
            // ============================================================
            Column {
                width: settingsRoot.colWidth
                spacing: 10

                Text {
                    text: "SYSTEM"
                    color: Theme.keyTextDim
                    font.pixelSize: 10
                    font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                // Autostart
                Row {
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Autostart:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 100
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 60; height: 28; radius: 4
                        color: KeyboardController.autostartEnabled
                               ? Theme.keyBackgroundModActive
                               : Theme.keyBackground

                        Text {
                            anchors.centerIn: parent
                            text: KeyboardController.autostartEnabled ? "On" : "Off"
                            color: Theme.keyText
                            font.pixelSize: 13
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: KeyboardController.setAutostartEnabled(!KeyboardController.autostartEnabled)
                        }
                    }
                }

                // Global shortcut
                Row {
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Shortcut:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 100
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 140
                        height: 28
                        radius: 4
                        color: Qt.lighter(Theme.keyboardBackground, 1.3)
                        border.color: Theme.keyTextDim
                        border.width: 1

                        TextEdit {
                            id: shortcutInput
                            anchors.fill: parent
                            anchors.margins: 6
                            color: Theme.keyText
                            selectionColor: Theme.keyBackgroundPressed
                            font.pixelSize: 12
                            verticalAlignment: TextEdit.AlignVCenter
                            text: KeyboardController.globalShortcut
                            onEditingFinished: {
                                if (text !== KeyboardController.globalShortcut)
                                    KeyboardController.setGlobalShortcut(text)
                            }
                        }
                    }
                }

                // Default screen
                Row {
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Screen:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 100
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 28; height: 28; radius: 4
                        color: decScreenMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                        Text { anchors.centerIn: parent; text: "-"; color: Theme.keyText; font.pixelSize: 14 }
                        MouseArea {
                            id: decScreenMa; anchors.fill: parent
                            onClicked: KeyboardController.setDefaultScreen(KeyboardController.defaultScreen - 1)
                        }
                    }

                    Text {
                        text: KeyboardController.defaultScreen
                        color: Theme.keyText
                        font.pixelSize: 13
                        width: 60
                        horizontalAlignment: Text.AlignHCenter
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Rectangle {
                        width: 28; height: 28; radius: 4
                        color: incScreenMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                        Text { anchors.centerIn: parent; text: "+"; color: Theme.keyText; font.pixelSize: 14 }
                        MouseArea {
                            id: incScreenMa; anchors.fill: parent
                            onClicked: KeyboardController.setDefaultScreen(KeyboardController.defaultScreen + 1)
                        }
                    }
                }

                // Whisper model path
                Column {
                    spacing: 4
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        text: "Whisper model:"
                        color: Theme.keyText
                        font.pixelSize: 13
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Row {
                        spacing: 4
                        anchors.horizontalCenter: parent.horizontalCenter

                        Rectangle {
                            width: Math.min(320, settingsRoot.colWidth - 60)
                            height: 28
                            radius: 4
                            color: Qt.lighter(Theme.keyboardBackground, 1.3)
                            border.color: Theme.keyTextDim
                            border.width: 1

                            TextEdit {
                                id: whisperPathInput
                                anchors.fill: parent
                                anchors.margins: 6
                                color: Theme.keyText
                                selectionColor: Theme.keyBackgroundPressed
                                font.pixelSize: 11
                                verticalAlignment: TextEdit.AlignVCenter
                                text: KeyboardController.whisperModelPath
                                onTextChanged: {
                                    if (text !== KeyboardController.whisperModelPath)
                                        KeyboardController.setWhisperModelPath(text)
                                }
                            }
                        }

                        Rectangle {
                            width: 36; height: 28; radius: 4
                            color: browseMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                            Text { anchors.centerIn: parent; text: "..."; color: Theme.keyText; font.pixelSize: 13; font.bold: true }
                            MouseArea {
                                id: browseMa; anchors.fill: parent
                                onClicked: KeyboardController.browseWhisperModel()
                            }
                        }
                    }
                }

                Item { width: 1; height: 10 }
            }
        }
    }
}
