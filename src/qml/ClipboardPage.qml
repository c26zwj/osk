import QtQuick

Rectangle {
    id: clipboardRoot
    visible: KeyboardController.clipboardPageVisible
    color: Theme.keyboardBackground
    onVisibleChanged: if (visible) filterInput.forceActiveFocus()

    property string filterText: ""

    // Block clicks from reaching the keyboard behind
    MouseArea { anchors.fill: parent }

    Column {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 6

        // Header
        Item {
            width: parent.width
            height: 28

            Text {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                text: "Clipboard"
                color: Theme.keyText
                font.pixelSize: 16
                font.bold: true
            }

            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 6

                Rectangle {
                    width: 60; height: 26; radius: 4
                    color: refreshMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                    Text { anchors.centerIn: parent; text: "Refresh"; color: Theme.keyText; font.pixelSize: 12 }
                    MouseArea {
                        id: refreshMa; anchors.fill: parent
                        onClicked: KeyboardController.refreshClipboardHistory()
                    }
                }
            }
        }

        // Filter box
        Rectangle {
            width: parent.width
            height: 30
            radius: 4
            color: Qt.lighter(Theme.keyboardBackground, 1.3)
            border.color: Theme.keyTextDim
            border.width: 1

            TextEdit {
                id: filterInput
                anchors.fill: parent
                anchors.margins: 6
                color: Theme.keyText
                selectionColor: Theme.keyBackgroundPressed
                font.pixelSize: 13
                verticalAlignment: TextEdit.AlignVCenter
                onTextChanged: clipboardRoot.filterText = text.toLowerCase()
            }

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                visible: filterInput.text.length === 0 && !filterInput.activeFocus
                text: "Filter..."
                color: Theme.keyTextDim
                font.pixelSize: 13
            }
        }

        // Clipboard entries list
        Rectangle {
            width: parent.width
            height: parent.height - 70
            radius: 4
            color: Qt.darker(Theme.keyboardBackground, 1.1)

            Flickable {
                anchors.fill: parent
                anchors.margins: 4
                clip: true
                contentHeight: listCol.height
                boundsBehavior: Flickable.StopAtBounds

                Column {
                    id: listCol
                    width: parent.width
                    spacing: 3

                    Repeater {
                        model: {
                            var items = KeyboardController.clipboardHistory;
                            if (clipboardRoot.filterText === "")
                                return items;
                            var f = clipboardRoot.filterText;
                            var result = [];
                            for (var i = 0; i < items.length; i++) {
                                if (items[i].toLowerCase().indexOf(f) >= 0)
                                    result.push(items[i]);
                            }
                            return result;
                        }

                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            width: listCol.width
                            height: 32
                            radius: 3
                            color: entryMa.containsMouse
                                   ? Qt.lighter(Theme.keyBackground, 1.1)
                                   : Theme.keyBackground

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 8
                                anchors.right: parent.right
                                anchors.rightMargin: 8
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.replace(/\n/g, " ")
                                color: Theme.keyText
                                font.pixelSize: 12
                                elide: Text.ElideRight
                                maximumLineCount: 1
                            }

                            MouseArea {
                                id: entryMa
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    filterInput.text = "";
                                    KeyboardController.insertClipboardEntry(modelData);
                                }
                            }
                        }
                    }

                    Text {
                        visible: KeyboardController.clipboardHistory.length === 0
                        text: "No clipboard history.\nKDE Klipper may not be running."
                        color: Theme.keyTextDim
                        font.pixelSize: 11
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        topPadding: 20
                    }
                }
            }
        }
    }
}
