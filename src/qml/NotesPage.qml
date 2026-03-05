import QtQuick

Rectangle {
    id: notesRoot
    visible: KeyboardController.notesPageVisible
    color: Theme.keyboardBackground

    onVisibleChanged: {
        if (visible)
            noteEdit.forceActiveFocus()
    }

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
                text: "Notes"
                color: Theme.keyText
                font.pixelSize: 16
                font.bold: true
            }

            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 6

                Rectangle {
                    width: 50; height: 26; radius: 4
                    color: clearMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                    Text { anchors.centerIn: parent; text: "Clear"; color: Theme.keyText; font.pixelSize: 12 }
                    MouseArea {
                        id: clearMa; anchors.fill: parent
                        onClicked: KeyboardController.setNotesContent("")
                    }
                }

                Rectangle {
                    width: 50; height: 26; radius: 4
                    color: copyAllMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                    Text { anchors.centerIn: parent; text: "Copy"; color: Theme.keyText; font.pixelSize: 12 }
                    MouseArea {
                        id: copyAllMa; anchors.fill: parent
                        onClicked: {
                            noteEdit.selectAll()
                            noteEdit.copy()
                            noteEdit.deselect()
                        }
                    }
                }
            }
        }

        // Text editor
        Rectangle {
            width: parent.width
            height: parent.height - 34
            radius: 4
            color: Qt.darker(Theme.keyboardBackground, 1.1)
            border.color: Theme.keyTextDim
            border.width: 1

            Flickable {
                id: noteFlickable
                anchors.fill: parent
                anchors.margins: 6
                clip: true
                contentWidth: width
                contentHeight: Math.max(noteEdit.implicitHeight, height)
                boundsBehavior: Flickable.StopAtBounds

                TextEdit {
                    id: noteEdit
                    width: noteFlickable.width
                    height: Math.max(noteFlickable.height, implicitHeight)
                    text: KeyboardController.notesContent
                    color: Theme.keyText
                    selectionColor: Theme.keyBackgroundPressed
                    font.pixelSize: 13
                    wrapMode: TextEdit.Wrap

                    onTextChanged: {
                        KeyboardController.setNotesContent(text)
                        // Keep cursor visible
                        var cursorRect = noteEdit.cursorRectangle
                        if (cursorRect.y < noteFlickable.contentY)
                            noteFlickable.contentY = cursorRect.y
                        else if (cursorRect.y + cursorRect.height > noteFlickable.contentY + noteFlickable.height)
                            noteFlickable.contentY = cursorRect.y + cursorRect.height - noteFlickable.height
                    }
                }
            }

            Text {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: 8
                visible: noteEdit.text.length === 0 && !noteEdit.activeFocus
                text: "Type your notes here..."
                color: Theme.keyTextDim
                font.pixelSize: 13
            }
        }
    }
}
