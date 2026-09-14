import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    width: 640
    height: 480
    visible: true
    required property var controller
    required property var mediaModel
    property string selectedText: controller.selectedName
    property string importError: ""

    Column {
        anchors.fill: parent
        spacing: 8
        Row {
            width: parent.width
            spacing: 8
            TextField {
                id: mediaPath
                objectName: "mediaPath"
                width: Math.max(80, parent.width - addButton.width - 8)
                placeholderText: "Local media path"
            }
            Button {
                id: addButton
                objectName: "addMediaButton"
                text: "Add"
                onClicked: {
                    if (root.controller.addMedia(mediaPath.text)) {
                        root.importError = ""
                        mediaPath.clear()
                    } else {
                        root.importError = "Cannot import this local file"
                    }
                }
            }
        }
        TextField {
            id: filterBox
            objectName: "filterBox"
            placeholderText: "Filter"
            onTextChanged: mediaModel.setFilter(text)
        }
        ListView {
            id: list
            objectName: "mediaList"
            width: parent.width
            height: 260
            model: mediaModel
            delegate: ItemDelegate {
                required property string displayName
                required property int row
                width: list.width
                text: displayName
                onClicked: controller.selectRow(row)
            }
        }
        Row {
            spacing: 8
            Button {
                id: play
                objectName: "playButton"
                text: controller.playing ? "Pause" : "Play"
                onClicked: controller.togglePlayback()
            }
            Label {
                id: selected
                objectName: "selectedLabel"
                text: root.selectedText
            }
        }
        Label {
            width: parent.width
            text: root.importError.length ? root.importError : root.controller.errorText
            wrapMode: Text.Wrap
        }
    }
}
