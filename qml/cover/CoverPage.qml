import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    Column {
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: Theme.paddingLarge
        }
        spacing: Theme.paddingSmall

        Label {
            width: parent.width
            text: "RadioInfo"
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeLarge
            truncationMode: TruncationMode.Fade
        }

        Label {
            width: parent.width
            text: "SIM " + (networkInfo.selectedSim + 1) + "  " + networkInfo.operatorName
            color: Theme.primaryColor
            font.pixelSize: Theme.fontSizeSmall
            truncationMode: TruncationMode.Fade
        }

        Label {
            width: parent.width
            text: networkInfo.lteAnchorActive
                  ? networkInfo.technology + (networkInfo.band !== "—" ? "  LTE " + networkInfo.band : "")
                  : networkInfo.technology + (networkInfo.band !== "—" ? "  " + networkInfo.band : "")
            color: Theme.primaryColor
            font.pixelSize: Theme.fontSizeMedium
            truncationMode: TruncationMode.Fade
        }

        Label {
            width: parent.width
            text: "RSRP  " + networkInfo.rsrp.split(" (")[0]
            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeSmall
            truncationMode: TruncationMode.Fade
        }

        Label {
            width: parent.width
            text: "SINR  " + networkInfo.sinr.split(" (")[0]
            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeSmall
            truncationMode: TruncationMode.Fade
        }

        Item {
            width: parent.width
            height: Theme.paddingMedium
        }

        Label {
            width: parent.width
            text: networkInfo.wifiSsid !== "—" ? qsTr("Wi-Fi") + "  " + networkInfo.wifiSsid : qsTr("Wi-Fi") + "  —"
            color: Theme.primaryColor
            font.pixelSize: Theme.fontSizeSmall
            truncationMode: TruncationMode.Fade
        }

        Label {
            width: parent.width
            text: networkInfo.wifiRssi !== "—" ? "RSSI  " + networkInfo.wifiRssi : ""
            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeSmall
            visible: text.length > 0
            truncationMode: TruncationMode.Fade
        }
    }

    Label {
        anchors {
            right: parent.right
            bottom: parent.bottom
            margins: Theme.paddingMedium
        }
        text: "v" + appVersion
        color: Theme.secondaryColor
        font.pixelSize: Theme.fontSizeTiny
    }
}
