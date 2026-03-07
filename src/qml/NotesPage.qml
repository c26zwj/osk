import QtQuick

Rectangle {
    id: notesRoot
    visible: KeyboardController.notesPageVisible
    color: Theme.keyboardBackground

    // Prevent re-entrant updates when loadCurrentNote sets editor text
    property bool loadingNote: false

    function loadCurrentNote() {
        loadingNote = true
        var notes = KeyboardController.notesList
        var idx = KeyboardController.currentNoteIndex
        if (idx >= 0 && idx < notes.length) {
            titleEdit.text = notes[idx].title || ""
            noteEdit.text  = notes[idx].content || ""
        }
        loadingNote = false
    }

    Component.onCompleted: loadCurrentNote()

    onVisibleChanged: {
        if (visible) {
            loadCurrentNote()
            noteEdit.forceActiveFocus()
        }
    }

    Connections {
        target: KeyboardController
        function onCurrentNoteIndexChanged() { notesRoot.loadCurrentNote() }
        function onNotesListChanged()        { notesRoot.loadCurrentNote() }
    }

    // Block clicks from reaching the keyboard behind
    MouseArea { anchors.fill: parent }

    // ── Tab bar ────────────────────────────────────────────────
    Item {
        id: tabBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 10
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        height: 30

        Flickable {
            id: tabFlick
            anchors.left: parent.left
            anchors.right: addBtn.left
            anchors.rightMargin: 4
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            contentWidth: tabRow.width
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            Row {
                id: tabRow
                spacing: 3
                height: tabFlick.height

                Repeater {
                    model: KeyboardController.notesList

                    delegate: Rectangle {
                        required property var modelData
                        required property int index

                        property bool isActive: index === KeyboardController.currentNoteIndex

                        height: tabRow.height
                        width: Math.min(tabLabel.implicitWidth + closeBtn.width + 16, 130)
                        radius: 4
                        color: isActive ? "#00414A" : Qt.darker("#00414A", 1.4)

                        // Tab title (clickable area to switch)
                        Text {
                            id: tabLabel
                            anchors.left: parent.left
                            anchors.leftMargin: 8
                            anchors.right: closeBtn.left
                            anchors.rightMargin: 2
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.title || "Untitled"
                            color: "#ffffff"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                        MouseArea {
                            anchors.left: parent.left
                            anchors.right: closeBtn.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            onClicked: KeyboardController.setCurrentNoteIndex(index)
                        }

                        // Close button
                        Rectangle {
                            id: closeBtn
                            anchors.right: parent.right
                            anchors.rightMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                            width: 16; height: 16; radius: 3
                            color: closeMa.containsMouse ? "#c0392b" : "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: "\u00d7"
                                color: closeMa.containsMouse ? "#ffffff" : Theme.keyTextDim
                                font.pixelSize: 13
                            }
                            MouseArea {
                                id: closeMa
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: KeyboardController.deleteNote(index)
                            }
                        }
                    }
                }
            }
        }

        // Add-page button
        Rectangle {
            id: addBtn
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 30; radius: 4
            color: addMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
            Text {
                anchors.centerIn: parent
                text: "+"
                color: Theme.keyText
                font.pixelSize: 18
            }
            MouseArea {
                id: addMa; anchors.fill: parent
                onClicked: KeyboardController.addNote()
            }
        }
    }

    // ── Title field ────────────────────────────────────────────
    Rectangle {
        id: titleField
        anchors.top: tabBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 6
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        height: 30
        radius: 4
        color: Qt.lighter(Theme.keyboardBackground, 1.3)
        border.color: Theme.keyTextDim
        border.width: 1

        TextEdit {
            id: titleEdit
            anchors.left: parent.left
            anchors.right: actionRow.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: 8
            anchors.rightMargin: 4
            color: Theme.keyText
            selectionColor: Theme.keyBackgroundPressed
            font.pixelSize: 13
            font.bold: true
            verticalAlignment: TextEdit.AlignVCenter
            onTextChanged: {
                if (!notesRoot.loadingNote)
                    KeyboardController.setNoteTitle(KeyboardController.currentNoteIndex, text)
            }
        }
        Text {
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            visible: titleEdit.text.length === 0
            text: "Untitled"
            color: Theme.keyTextDim
            font.pixelSize: 13
        }

        Row {
            id: actionRow
            anchors.right: parent.right
            anchors.rightMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4

            Rectangle {
                width: 44; height: 22; radius: 3
                color: copyMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                Text { anchors.centerIn: parent; text: "Copy"; color: Theme.keyText; font.pixelSize: 11 }
                MouseArea {
                    id: copyMa; anchors.fill: parent
                    onClicked: {
                        noteEdit.selectAll()
                        noteEdit.copy()
                        noteEdit.deselect()
                    }
                }
            }
            Rectangle {
                width: 44; height: 22; radius: 3
                color: clearMa.pressed ? Theme.keyBackgroundPressed : Theme.keyBackground
                Text { anchors.centerIn: parent; text: "Clear"; color: Theme.keyText; font.pixelSize: 11 }
                MouseArea {
                    id: clearMa; anchors.fill: parent
                    onClicked: {
                        KeyboardController.setNoteContent(KeyboardController.currentNoteIndex, "")
                        noteEdit.text = ""
                    }
                }
            }
        }
    }

    // ── Content area ───────────────────────────────────────────
    Rectangle {
        anchors.top: titleField.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.topMargin: 6
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.bottomMargin: 10
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
                color: Theme.keyText
                selectionColor: Theme.keyBackgroundPressed
                font.pixelSize: 13
                wrapMode: TextEdit.Wrap
                onTextChanged: {
                    if (notesRoot.loadingNote) return
                    KeyboardController.setNoteContent(KeyboardController.currentNoteIndex, text)
                    var cr = noteEdit.cursorRectangle
                    if (cr.y < noteFlickable.contentY)
                        noteFlickable.contentY = cr.y
                    else if (cr.y + cr.height > noteFlickable.contentY + noteFlickable.height)
                        noteFlickable.contentY = cr.y + cr.height - noteFlickable.height
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
