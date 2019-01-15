import QtQuick 2.0

Item {
	id: root

	property bool brokerConnected: false
	property string name: "Test predator"
	property string shvPath: "/shv/cze/prg/eline/exotics/001"

	width: 200
	height: 60
	Row {
		id: row1
		anchors.rightMargin: 10
		anchors.right: parent.right
		anchors.verticalCenter: parent.verticalCenter
		anchors.left: parent.left
		anchors.leftMargin: 10
		spacing: 10
		Rectangle {
			width: root.height - row1.spacing*  2
			height: width
			radius: height / 3
			anchors.verticalCenter: parent.verticalCenter
			color: brokerConnected? "green": "red"
		}
		Column {
			id: column
			Text {
				width: root.width - column.x
				text: root.name
				elide: Text.ElideRight
				font.bold: true
			}
			Text {
				width: root.width - column.x
				text: root.shvPath
				elide: Text.ElideRight
			}
		}
	}
}
