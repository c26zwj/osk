import QtQuick

Rectangle {
    id: shortcutsRoot
    visible: KeyboardController.shortcutPageVisible
    color: Theme.keyboardBackground

    property bool dialogOpen: false
    property int editingIndex: -1
    property int selectedIndex: -1
    property int confirmDeleteIndex: -1

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
                text: "Shortcuts"
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
                    color: addMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                    Text { anchors.centerIn: parent; text: "Add"; color: Theme.keyText; font.pixelSize: 12 }
                    MouseArea {
                        id: addMa; anchors.fill: parent
                        onClicked: {
                            shortcutInput.text = "";
                            expansionInput.text = "";
                            shortcutsRoot.editingIndex = -1;
                            shortcutsRoot.dialogOpen = true;
                            KeyboardController.setShortcutDialogOpen(true);
                            shortcutInput.forceActiveFocus();
                        }
                    }
                }
            }
        }

        // Two-panel content
        Row {
            width: parent.width
            height: parent.height - 40
            spacing: 6

            // Left panel: shortcut list
            Rectangle {
                width: Math.round(parent.width * 0.4)
                height: parent.height
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
                            model: KeyboardController.shortcuts

                            delegate: Rectangle {
                                required property var modelData
                                required property int index
                                width: listCol.width
                                height: 28
                                radius: 3
                                color: shortcutsRoot.selectedIndex === index
                                       ? Theme.keyBackgroundPressed
                                       : itemMa.containsMouse ? Qt.lighter(Theme.keyBackground, 1.1) : Theme.keyBackground

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 8
                                    anchors.right: itemBtnRow.left
                                    anchors.rightMargin: 4
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData.shortcut || ""
                                    color: Theme.keyText
                                    font.pixelSize: 12
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                Row {
                                    id: itemBtnRow
                                    anchors.right: parent.right
                                    anchors.rightMargin: 4
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 2

                                    Rectangle {
                                        width: 28; height: 22; radius: 3
                                        color: editItemMa.pressed ? Theme.keyBackgroundPressed : Qt.lighter(Theme.keyBackground, 1.3)
                                        Text { anchors.centerIn: parent; text: "Edit"; color: Theme.keyText; font.pixelSize: 9 }
                                        MouseArea {
                                            id: editItemMa; anchors.fill: parent
                                            onClicked: {
                                                shortcutInput.text = modelData.shortcut || "";
                                                expansionInput.text = modelData.expansion || "";
                                                shortcutsRoot.editingIndex = index;
                                                shortcutsRoot.dialogOpen = true;
                                                KeyboardController.setShortcutDialogOpen(true);
                                                shortcutInput.forceActiveFocus();
                                            }
                                        }
                                    }

                                    Rectangle {
                                        width: 22; height: 22; radius: 3
                                        color: delItemMa.pressed ? "#c0392b" : "transparent"
                                        Text {
                                            anchors.centerIn: parent; text: "\u2715"
                                            color: delItemMa.pressed ? "#ffffff" : Theme.keyTextDim; font.pixelSize: 10
                                        }
                                        MouseArea {
                                            id: delItemMa; anchors.fill: parent
                                            onClicked: shortcutsRoot.confirmDeleteIndex = index
                                        }
                                    }
                                }

                                MouseArea {
                                    id: itemMa
                                    anchors.fill: parent
                                    anchors.rightMargin: itemBtnRow.width + 8
                                    hoverEnabled: true
                                    onClicked: shortcutsRoot.selectedIndex = index
                                }
                            }
                        }

                        Text {
                            visible: KeyboardController.shortcuts.length === 0
                            text: "No shortcuts yet.\nTap \"Add\" to create one."
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

            // Right panel: expansion preview
            Rectangle {
                width: parent.width - Math.round(parent.width * 0.4) - 6
                height: parent.height
                radius: 4
                color: Qt.darker(Theme.keyboardBackground, 1.1)

                Flickable {
                    anchors.fill: parent
                    anchors.margins: 8
                    clip: true
                    contentHeight: expansionText.contentHeight
                    boundsBehavior: Flickable.StopAtBounds

                    Text {
                        id: expansionText
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: Theme.keyText
                        font.pixelSize: 12
                        text: {
                            if (shortcutsRoot.selectedIndex >= 0
                                && shortcutsRoot.selectedIndex < KeyboardController.shortcuts.length) {
                                return KeyboardController.shortcuts[shortcutsRoot.selectedIndex].expansion || "";
                            }
                            return "";
                        }
                    }
                }

                Text {
                    visible: shortcutsRoot.selectedIndex < 0
                             || shortcutsRoot.selectedIndex >= KeyboardController.shortcuts.length
                    anchors.centerIn: parent
                    text: "Select a shortcut\nto see its expansion"
                    color: Theme.keyTextDim
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }

    // Confirm delete dialog
    Rectangle {
        visible: shortcutsRoot.confirmDeleteIndex >= 0
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.75)
        MouseArea { anchors.fill: parent }

        Column {
            anchors.centerIn: parent
            spacing: 12

            Text {
                text: "Delete this shortcut?"
                color: Theme.keyText
                font.pixelSize: 15
                font.bold: true
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                width: Math.min(shortcutsRoot.width - 80, 300)
                text: {
                    var idx = shortcutsRoot.confirmDeleteIndex;
                    if (idx >= 0 && idx < KeyboardController.shortcuts.length) {
                        var e = KeyboardController.shortcuts[idx];
                        return (e.shortcut || "") + " \u2192 " + (e.expansion || "");
                    }
                    return "";
                }
                color: Theme.keyTextDim
                font.pixelSize: 11
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Row {
                spacing: 10
                anchors.horizontalCenter: parent.horizontalCenter

                Rectangle {
                    width: 70; height: 28; radius: 4
                    color: yesDelMa.pressed ? "#c0392b" : Theme.keyBackground
                    Text { anchors.centerIn: parent; text: "Delete"; color: Theme.keyText; font.pixelSize: 12 }
                    MouseArea {
                        id: yesDelMa; anchors.fill: parent
                        onClicked: {
                            var idx = shortcutsRoot.confirmDeleteIndex;
                            if (shortcutsRoot.selectedIndex === idx)
                                shortcutsRoot.selectedIndex = -1;
                            else if (shortcutsRoot.selectedIndex > idx)
                                shortcutsRoot.selectedIndex--;
                            KeyboardController.removeShortcut(idx);
                            shortcutsRoot.confirmDeleteIndex = -1;
                        }
                    }
                }

                Rectangle {
                    width: 70; height: 28; radius: 4
                    color: noDelMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                    Text { anchors.centerIn: parent; text: "Cancel"; color: Theme.keyText; font.pixelSize: 12 }
                    MouseArea {
                        id: noDelMa; anchors.fill: parent
                        onClicked: shortcutsRoot.confirmDeleteIndex = -1
                    }
                }
            }
        }
    }

    // Add/Edit shortcut dialog
    Rectangle {
        visible: shortcutsRoot.dialogOpen
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.75)
        MouseArea { anchors.fill: parent }

        Column {
            anchors.centerIn: parent
            spacing: 8
            width: Math.min(parent.width - 40, 400)

            Text {
                text: shortcutsRoot.editingIndex >= 0 ? "Edit Shortcut" : "Add Shortcut"
                color: Theme.keyText
                font.pixelSize: 16
                font.bold: true
                anchors.horizontalCenter: parent.horizontalCenter
            }

            // Trigger field
            Column {
                width: parent.width
                spacing: 2

                Text { text: "Trigger"; color: Theme.keyTextDim; font.pixelSize: 11 }

                Rectangle {
                    width: parent.width
                    height: 30
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
                        font.pixelSize: 13
                        verticalAlignment: TextEdit.AlignVCenter
                        Keys.onReturnPressed: expansionInput.forceActiveFocus()
                        Keys.onEnterPressed: expansionInput.forceActiveFocus()
                    }
                }
            }

            // Expansion field
            Column {
                width: parent.width
                spacing: 2

                Text { text: "Expansion"; color: Theme.keyTextDim; font.pixelSize: 11 }

                Rectangle {
                    width: parent.width
                    height: 70
                    radius: 4
                    color: Qt.lighter(Theme.keyboardBackground, 1.3)
                    border.color: Theme.keyTextDim
                    border.width: 1

                    Flickable {
                        anchors.fill: parent
                        anchors.margins: 6
                        contentHeight: expansionInput.contentHeight
                        clip: true

                        TextEdit {
                            id: expansionInput
                            width: parent.width
                            color: Theme.keyText
                            selectionColor: Theme.keyBackgroundPressed
                            font.pixelSize: 13
                            wrapMode: TextEdit.Wrap
                        }
                    }
                }
            }

            // Buttons
            Row {
                spacing: 8
                anchors.horizontalCenter: parent.horizontalCenter

                Rectangle {
                    width: 70; height: 28; radius: 4
                    color: saveMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                    Text { anchors.centerIn: parent; text: "Save"; color: Theme.keyText; font.pixelSize: 12 }
                    MouseArea {
                        id: saveMa; anchors.fill: parent
                        onClicked: {
                            if (shortcutInput.text.length > 0 && expansionInput.text.length > 0) {
                                if (shortcutsRoot.editingIndex >= 0)
                                    KeyboardController.editShortcut(shortcutsRoot.editingIndex,
                                                                     shortcutInput.text, expansionInput.text);
                                else
                                    KeyboardController.addShortcut(shortcutInput.text, expansionInput.text);
                            }
                            shortcutInput.text = "";
                            expansionInput.text = "";
                            shortcutsRoot.dialogOpen = false;
                            KeyboardController.setShortcutDialogOpen(false);
                        }
                    }
                }

                Rectangle {
                    width: 70; height: 28; radius: 4
                    color: cancelMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                    Text { anchors.centerIn: parent; text: "Cancel"; color: Theme.keyText; font.pixelSize: 12 }
                    MouseArea {
                        id: cancelMa; anchors.fill: parent
                        onClicked: {
                            shortcutInput.text = "";
                            expansionInput.text = "";
                            shortcutsRoot.dialogOpen = false;
                            KeyboardController.setShortcutDialogOpen(false);
                        }
                    }
                }
            }
        }
    }
}
