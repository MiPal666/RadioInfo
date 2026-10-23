import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    property int selectedSection: 0   // 0 = mobile, 1 = Wi-Fi

    SilicaFlickable {
        id: flickable
        anchors.fill: parent
        contentHeight: contentColumn.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: networkInfo.refreshing ? qsTr("Refreshing…") : qsTr("Refresh")
                enabled: !networkInfo.refreshing
                onClicked: networkInfo.refresh()
            }
        }

        Column {
            id: contentColumn
            width: parent.width

            Column {
                id: headerArea
                width: parent.width

                PageHeader {
                    title: "FelisRadioInfo"
                }

            }

            // The real tab bar is an overlay (stickyTabs) below. This spacer
            // reserves its natural position in the scrollable content.
            Item {
                width: parent.width
                height: Theme.itemSizeSmall
            }

            Column {
                id: mobileSection
                width: parent.width
                visible: page.selectedSection === 0

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
            }

            Column {
                id: wifiSection
                width: parent.width
                visible: page.selectedSection === 1

                DetailItem { label: qsTr("Interface"); value: networkInfo.wifiInterface }
                DetailItem { label: "SSID"; value: networkInfo.wifiSsid }
                DetailItem { label: "BSSID"; value: networkInfo.wifiBssid }
                DetailItem { label: qsTr("Wi-Fi standard"); value: networkInfo.wifiStandard }
                DetailItem { label: qsTr("Band"); value: networkInfo.wifiBand }
                DetailItem { label: qsTr("Channel"); value: networkInfo.wifiChannel }
                DetailItem { label: qsTr("Channel width"); value: networkInfo.wifiChannelWidth }
                DetailItem { label: qsTr("Frequency"); value: networkInfo.wifiFrequency }
                DetailItem { label: "RSSI"; value: networkInfo.wifiRssi }
                DetailItem { label: "RX rate"; value: networkInfo.wifiRxRate }
                DetailItem { label: "TX rate"; value: networkInfo.wifiTxRate }
                DetailItem { label: qsTr("Spatial streams (NSS)"); value: networkInfo.wifiNss }
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
            }


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
                    width: parent.width - versionLabel.width
                           - Theme.paddingMedium
                           - (networkInfo.refreshing ? Theme.itemSizeSmall : 0)
                    wrapMode: Text.Wrap
                    color: Theme.secondaryHighlightColor
                    text: networkInfo.status
                }

                Label {
                    id: versionLabel
                    anchors.verticalCenter: statusLabel.verticalCenter
                    text: qsTr("Version") + " " + appVersion
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                }
            }

            Item { width: 1; height: Theme.paddingLarge }
        }

        VerticalScrollDecorator { }
    }

    // Sticky tab bar. At the top of the page it sits below the title/version.
    // While scrolling it sticks to the top edge. During a pull-down it moves
    // down with the content, so the native pulley menu is revealed above it
    // instead of covering it.
    Item {
        id: stickyTabs
        width: parent.width
        height: Theme.itemSizeSmall
        y: Math.max(0, headerArea.height - flickable.contentY)
        z: 100

        Rectangle {
            anchors.fill: parent
            color: Theme.highlightBackgroundColor
            opacity: 0.12
        }

        Row {
            anchors.fill: parent

            BackgroundItem {
                id: mobileTab
                width: parent.width / 2
                height: parent.height
                onClicked: {
                    page.selectedSection = 0
                    flickable.contentY = 0
                }

                Label {
                    anchors.centerIn: parent
                    text: qsTr("Mobile network")
                    color: page.selectedSection === 0 ? Theme.highlightColor : Theme.primaryColor
                    font.bold: page.selectedSection === 0
                    font.pixelSize: Theme.fontSizeMedium
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 2
                    color: Theme.highlightColor
                    visible: page.selectedSection === 0
                }
            }

            BackgroundItem {
                id: wifiTab
                width: parent.width / 2
                height: parent.height
                onClicked: {
                    page.selectedSection = 1
                    flickable.contentY = 0
                }

                Label {
                    anchors.centerIn: parent
                    text: qsTr("Wi-Fi")
                    color: page.selectedSection === 1 ? Theme.highlightColor : Theme.primaryColor
                    font.bold: page.selectedSection === 1
                    font.pixelSize: Theme.fontSizeMedium
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 2
                    color: Theme.highlightColor
                    visible: page.selectedSection === 1
                }
            }
        }
    }
}
