import QtQuick 2.9
import QtQuick.Window 2.3
import QtQuick.Controls 2.5

Window {
    id: window
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello World")

    Column {
        anchors.fill: parent
        padding: 8
        spacing: 8

        Rectangle {
            height: 70
            border.color: "#000000"
            anchors {
                left: parent.left
                leftMargin: 8
                right: parent.right
                rightMargin: 8
            }

            TextArea {
                id: edSchema
                anchors {
                    fill: parent
                    margins: 1
                }
                focus: true
                wrapMode: Text.WordWrap
                font.pointSize: 14
                text: qsTr("E:\\GameData.xsd")
                placeholderText: qsTr("schema file fullname")
                KeyNavigation.priority: KeyNavigation.BeforeItem
                KeyNavigation.tab: edXML
            }
        }

        Rectangle {
            height: 70
            border.color: "#000000"
            anchors {
                left: parent.left
                leftMargin: 8
                right: parent.right
                rightMargin: 8
            }

            TextArea {
                id: edXML
                anchors {
                    fill: parent
                    margins: 1
                }
                focus: true
                wrapMode: Text.WordWrap
                font.pointSize: 14
                text: qsTr("E:\\GameData.xml")
                placeholderText: qsTr("XML file fullname")
                KeyNavigation.priority: KeyNavigation.BeforeItem
                KeyNavigation.tab: bOk
            }
        }

        Button {
            id: bOk
            width: 100
            height: 26
            text: "Test"
            onClicked: {                
                laRes.text = backend.test(edSchema.text, edXML.text);
            }
        }

        Label {
            id: laRes
            anchors {
                left: parent.left
                leftMargin: 8
                right: parent.right
                rightMargin: 8
            }
            wrapMode: Label.WordWrap
        }
    }
}
