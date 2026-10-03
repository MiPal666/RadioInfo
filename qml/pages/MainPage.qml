import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: networkInfo.refreshing ? qsTr("Refreshing…") : qsTr("Refresh")
                enabled: !networkInfo.refreshing
                onClicked: networkInfo.refresh()
            }
        }

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: networkInfo.refreshing ? qsTr("RadioInfo — refreshing…") : "RadioInfo"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                horizontalAlignment: Text.AlignRight
                text: qsTr("Version") + " " + appVersion
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
            }

            ComboBox {
                width: parent.width
                label: qsTr("Language")
                currentIndex: languageManager.currentIndex

                menu: ContextMenu {
                    MenuItem { text: qsTr("System") }
                    MenuItem { text: "English" }
                    MenuItem { text: "Čeština" }
                }

                onCurrentIndexChanged: {
                    if (currentIndex !== languageManager.currentIndex)
                        languageManager.setCurrentIndex(currentIndex)
                }
            }

            SectionHeader { text: qsTr("Mobile network") }

            ComboBox {
                width: parent.width
                label: "SIM"
                currentIndex: networkInfo.selectedSim
                enabled: networkInfo.simCount > 1 && !networkInfo.refreshing

                menu: ContextMenu {
                    MenuItem {
                        text: networkInfo.sim1Label
                        visible: networkInfo.simCount > 0
                    }
                    MenuItem {
                        text: networkInfo.sim2Label
                        visible: networkInfo.simCount > 1
                    }
                }

                onCurrentIndexChanged: {
                    if (currentIndex >= 0 && currentIndex !== networkInfo.selectedSim)
                        networkInfo.selectSim(currentIndex)
                }
            }

            DetailItem { label: qsTr("Modem"); value: networkInfo.modemPath }
            DetailItem { label: qsTr("Operator"); value: networkInfo.operatorName }
            DetailItem { label: "MCC / MNC"; value: networkInfo.mccMnc }
            DetailItem { label: qsTr("Technology"); value: networkInfo.technology }
            DetailItem { label: networkInfo.bandLabel; value: networkInfo.band }
            DetailItem { label: networkInfo.earfcnLabel; value: networkInfo.earfcn }
            DetailItem {
                visible: networkInfo.nrActive
                label: qsTr("NR band")
                value: networkInfo.nrBand
            }
            DetailItem { label: qsTr("Cell ID"); value: networkInfo.cellId }
            DetailItem { label: "eNB"; value: networkInfo.eNbId }
            DetailItem { label: "CID"; value: networkInfo.localCellId }
            DetailItem { label: "TAC"; value: networkInfo.tac }
            DetailItem { label: "PCI"; value: networkInfo.pci }
            DetailItem { label: networkInfo.lteAnchorActive ? qsTr("LTE RSRP") : qsTr("RSRP"); value: networkInfo.rsrp }
            DetailItem { label: networkInfo.lteAnchorActive ? qsTr("LTE RSRQ") : qsTr("RSRQ"); value: networkInfo.rsrq }
            DetailItem { label: networkInfo.lteAnchorActive ? qsTr("LTE SINR") : qsTr("SINR"); value: networkInfo.sinr }
            DetailItem { label: "CQI"; value: networkInfo.cqi }
            DetailItem { label: qsTr("Timing Advance"); value: networkInfo.timingAdvance }
            DetailItem { label: "IMS"; value: networkInfo.ims }

            SectionHeader { text: qsTr("Mobile IP interfaces") }

            Repeater {
                model: networkInfo.mobileIpEntries
                delegate: DetailItem {
                    label: modelData.name + " " + modelData.family
                    value: modelData.address
                }
            }

            Label {
                visible: networkInfo.mobileIpEntries.length === 0
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                horizontalAlignment: Text.AlignRight
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                text: qsTr("No active mobile IP interface")
            }

            SectionHeader { text: qsTr("Wi-Fi") }

            DetailItem { label: qsTr("Interface"); value: networkInfo.wifiInterface }
            DetailItem { label: "SSID"; value: networkInfo.wifiSsid }
            DetailItem { label: "BSSID"; value: networkInfo.wifiBssid }
            DetailItem { label: qsTr("Band"); value: networkInfo.wifiBand }
            DetailItem { label: qsTr("Channel"); value: networkInfo.wifiChannel }
            DetailItem { label: qsTr("Frequency"); value: networkInfo.wifiFrequency }
            DetailItem { label: "RSSI"; value: networkInfo.wifiRssi }
            DetailItem { label: "RX rate"; value: networkInfo.wifiRxRate }
            DetailItem { label: "TX rate"; value: networkInfo.wifiTxRate }
            DetailItem { label: "IPv4"; value: networkInfo.wifiIp }
            DetailItem { label: "IPv6"; value: networkInfo.wifiIpv6 }
            DetailItem { label: qsTr("IPv4 gateway"); value: networkInfo.wifiGateway }
            DetailItem { label: qsTr("IPv6 gateway"); value: networkInfo.wifiGateway6 }
            DetailItem { label: "DNS"; value: networkInfo.wifiDns }

            Label {
                visible: networkInfo.wifiStatus !== "OK"
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                text: "Wi-Fi: " + networkInfo.wifiStatus
            }

            SectionHeader { text: qsTr("Status") }

            Row {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingMedium

                BusyIndicator {
                    size: BusyIndicatorSize.Small
                    running: networkInfo.refreshing
                    visible: networkInfo.refreshing
                    anchors.verticalCenter: statusLabel.verticalCenter
                }

                Label {
                    id: statusLabel
                    width: parent.width - (networkInfo.refreshing ? Theme.itemSizeSmall : 0)
                    wrapMode: Text.Wrap
                    color: Theme.secondaryHighlightColor
                    text: networkInfo.status
                }
            }

            Item { width: 1; height: Theme.paddingLarge }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: networkInfo.refreshing ? qsTr("Refreshing…") : qsTr("Refresh")
                enabled: !networkInfo.refreshing
                onClicked: networkInfo.refresh()
            }
        }
    }
}
