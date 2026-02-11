import QtQuick

Rectangle {
    id: settingsRoot
    visible: KeyboardController.settingsVisible
    color: Theme.keyboardBackground

    // Block clicks from reaching the keyboard behind
    MouseArea { anchors.fill: parent }

    Column {
        anchors.centerIn: parent
        spacing: 14

        // Title
        Text {
            text: "Settings"
            color: Theme.keyText
            font.pixelSize: 18
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
    }
}
