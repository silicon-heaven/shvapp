import QtQuick 2.0
import QtPositioning 5.5
import QtLocation 5.6

Rectangle {
	id: rectangle
	anchors.fill: parent

	Plugin {
		id: myPlugin
		name: "osm" // "mapboxgl", "esri", ...
		//specify plugin parameters if necessary
		//PluginParameter {...}
		//PluginParameter {...}
		//...
	}

	property variant locationPrague: QtPositioning.coordinate(50.0835494, 14.4341414)
	width: 1024
	height: 768
	Map {
		id: map
		anchors.left: listView.right
		anchors.right: parent.right
		anchors.bottom: parent.bottom
		anchors.top: parent.top
		plugin: myPlugin;
		center: locationPrague
		zoomLevel: 9

		MapItemView {
			model: poiModel
			delegate: MapQuickItem {
				coordinate: QtPositioning.coordinate(GpsRole[0], GpsRole[1])

				anchorPoint.x: image.width * 0.5
				anchorPoint.y: image.height
				z: BrokerConnectedRole? 2: 1

				sourceItem: Column {
					Image {
						id: image;
						source: BrokerConnectedRole? "images/marker-on.svg": "images/marker-off.svg"
						fillMode: Image.PreserveAspectFit
						height: 60
					}
					Text { text: NameRole; font.bold: true }
					Text { text: ShvPathRole }
				}
			}
		}

		onZoomLevelChanged: {
			//console.debug("zoom level:", map.zoomLevel)
		}
	}

	ListView {
		id: listView
		width: 400
		anchors.left: parent.left
		anchors.bottom: parent.bottom
		anchors.top: parent.top

		highlight: Rectangle { color: "lightsteelblue"; radius: 5 }
		focus: true

		delegate: SiteListRow {
			width: listView.width
			name: NameRole
			shvPath: ShvPathRole
			brokerConnected: BrokerConnectedRole

			MouseArea {
				anchors.fill: parent
				onClicked: listView.currentIndex = index
			}
		}
		model: poiModel

		onCurrentIndexChanged: {
			var gps = poiModel.value(currentIndex, "GpsRole");
			//console.log("GPS changed:", gps)
			map.center = QtPositioning.coordinate(gps[0], gps[1])
		}
	}
	/*
	Connections {
		target: poiModel
		onDataChanged: {
			console.log("DataChanged received", topLeft, bottomRight, roles);
			console.log("new data", poiModel.data(topLeft, bottomRight, roles));
		}
	}
	*/
}

