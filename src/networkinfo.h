#pragma once

#include <QObject>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusVariant>
#include <QList>
#include <QSet>
#include <QStringList>
#include <QVariantMap>

struct OfonoModem
{
    QDBusObjectPath path;
    QVariantMap properties;
};

Q_DECLARE_METATYPE(OfonoModem)
typedef QList<OfonoModem> OfonoModemList;
Q_DECLARE_METATYPE(OfonoModemList)

QDBusArgument &operator<<(QDBusArgument &argument, const OfonoModem &modem);
const QDBusArgument &operator>>(const QDBusArgument &argument, OfonoModem &modem);

class NetworkInfo : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString status READ status NOTIFY dataChanged)
    Q_PROPERTY(bool refreshing READ refreshing NOTIFY refreshingChanged)

    Q_PROPERTY(int simCount READ simCount NOTIFY simListChanged)
    Q_PROPERTY(int selectedSim READ selectedSim NOTIFY selectedSimChanged)
    Q_PROPERTY(QString selectedSimLabel READ selectedSimLabel NOTIFY simListChanged)
    Q_PROPERTY(QString sim1Label READ sim1Label NOTIFY simListChanged)
    Q_PROPERTY(QString sim2Label READ sim2Label NOTIFY simListChanged)
    Q_PROPERTY(QString modemPath READ modemPath NOTIFY selectedSimChanged)

    Q_PROPERTY(QString operatorName READ operatorName NOTIFY dataChanged)
    Q_PROPERTY(QString mccMnc READ mccMnc NOTIFY dataChanged)
    Q_PROPERTY(QString technology READ technology NOTIFY dataChanged)
    Q_PROPERTY(bool nrActive READ nrActive NOTIFY dataChanged)
    Q_PROPERTY(bool lteAnchorActive READ lteAnchorActive NOTIFY dataChanged)
    Q_PROPERTY(QString band READ band NOTIFY dataChanged)
    Q_PROPERTY(QString bandLabel READ bandLabel NOTIFY dataChanged)
    Q_PROPERTY(QString earfcn READ earfcn NOTIFY dataChanged)
    Q_PROPERTY(QString earfcnLabel READ earfcnLabel NOTIFY dataChanged)
    Q_PROPERTY(QString nrBand READ nrBand NOTIFY dataChanged)
    Q_PROPERTY(QString cellId READ cellId NOTIFY dataChanged)
    Q_PROPERTY(QString eNbId READ eNbId NOTIFY dataChanged)
    Q_PROPERTY(QString localCellId READ localCellId NOTIFY dataChanged)
    Q_PROPERTY(QString tac READ tac NOTIFY dataChanged)
    Q_PROPERTY(QString pci READ pci NOTIFY dataChanged)
    Q_PROPERTY(QString rsrp READ rsrp NOTIFY dataChanged)
    Q_PROPERTY(QString rsrq READ rsrq NOTIFY dataChanged)
    Q_PROPERTY(QString sinr READ sinr NOTIFY dataChanged)
    Q_PROPERTY(QString cqi READ cqi NOTIFY dataChanged)
    Q_PROPERTY(QString timingAdvance READ timingAdvance NOTIFY dataChanged)
    Q_PROPERTY(QString ims READ ims NOTIFY dataChanged)
    Q_PROPERTY(QVariantList mobileIpEntries READ mobileIpEntries NOTIFY dataChanged)

    Q_PROPERTY(QString wifiInterface READ wifiInterface NOTIFY dataChanged)
    Q_PROPERTY(QString wifiSsid READ wifiSsid NOTIFY dataChanged)
    Q_PROPERTY(QString wifiBssid READ wifiBssid NOTIFY dataChanged)
    Q_PROPERTY(QString wifiBand READ wifiBand NOTIFY dataChanged)
    Q_PROPERTY(QString wifiChannel READ wifiChannel NOTIFY dataChanged)
    Q_PROPERTY(QString wifiFrequency READ wifiFrequency NOTIFY dataChanged)
    Q_PROPERTY(QString wifiRssi READ wifiRssi NOTIFY dataChanged)
    Q_PROPERTY(QString wifiRxRate READ wifiRxRate NOTIFY dataChanged)
    Q_PROPERTY(QString wifiTxRate READ wifiTxRate NOTIFY dataChanged)
    Q_PROPERTY(QString wifiIp READ wifiIp NOTIFY dataChanged)
    Q_PROPERTY(QString wifiIpv6 READ wifiIpv6 NOTIFY dataChanged)
    Q_PROPERTY(QString wifiGateway READ wifiGateway NOTIFY dataChanged)
    Q_PROPERTY(QString wifiGateway6 READ wifiGateway6 NOTIFY dataChanged)
    Q_PROPERTY(QString wifiDns READ wifiDns NOTIFY dataChanged)
    Q_PROPERTY(QString wifiStatus READ wifiStatus NOTIFY dataChanged)

public:
    explicit NetworkInfo(QObject *parent = nullptr);

    QString status() const { return m_status; }
    bool refreshing() const { return m_refreshing; }

    int simCount() const { return m_modems.size(); }
    int selectedSim() const { return m_selectedSim; }
    QString selectedSimLabel() const {
        return (m_selectedSim >= 0 && m_selectedSim < m_simLabels.size())
                ? m_simLabels.at(m_selectedSim) : QStringLiteral("SIM");
    }
    QString sim1Label() const {
        return m_simLabels.size() > 0 ? m_simLabels.at(0) : QStringLiteral("SIM 1");
    }
    QString sim2Label() const {
        return m_simLabels.size() > 1 ? m_simLabels.at(1) : QStringLiteral("SIM 2");
    }
    QString modemPath() const { return m_modemPath.isEmpty() ? QStringLiteral("—") : m_modemPath; }

    QString operatorName() const { return m_operatorName; }
    QString mccMnc() const { return m_mccMnc; }
    QString technology() const { return m_technology; }
    bool nrActive() const { return m_nrActive; }
    bool lteAnchorActive() const { return m_nrActive && m_servingCellType == QStringLiteral("lte"); }
    QString band() const { return m_band; }
    QString bandLabel() const { return m_bandLabel; }
    QString earfcn() const { return m_earfcn; }
    QString earfcnLabel() const { return m_earfcnLabel; }
    QString nrBand() const { return m_nrBand; }
    QString cellId() const { return m_cellId; }
    QString eNbId() const { return m_eNbId; }
    QString localCellId() const { return m_localCellId; }
    QString tac() const { return m_tac; }
    QString pci() const { return m_pci; }
    QString rsrp() const { return m_rsrp; }
    QString rsrq() const { return m_rsrq; }
    QString sinr() const { return m_sinr; }
    QString cqi() const { return m_cqi; }
    QString timingAdvance() const { return m_timingAdvance; }
    QString ims() const { return m_ims; }
    QVariantList mobileIpEntries() const { return m_mobileIpEntries; }

    QString wifiInterface() const { return m_wifiInterface; }
    QString wifiSsid() const { return m_wifiSsid; }
    QString wifiBssid() const { return m_wifiBssid; }
    QString wifiBand() const { return m_wifiBand; }
    QString wifiChannel() const { return m_wifiChannel; }
    QString wifiFrequency() const { return m_wifiFrequency; }
    QString wifiRssi() const { return m_wifiRssi; }
    QString wifiRxRate() const { return m_wifiRxRate; }
    QString wifiTxRate() const { return m_wifiTxRate; }
    QString wifiIp() const { return m_wifiIp; }
    QString wifiIpv6() const { return m_wifiIpv6; }
    QString wifiGateway() const { return m_wifiGateway; }
    QString wifiGateway6() const { return m_wifiGateway6; }
    QString wifiDns() const { return m_wifiDns; }
    QString wifiStatus() const { return m_wifiStatus; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void selectSim(int index);
    Q_INVOKABLE void retranslate();

signals:
    void dataChanged();
    void refreshingChanged();
    void simListChanged();
    void selectedSimChanged();

private slots:
    void cellsAdded(const QList<QDBusObjectPath> &cells);
    void cellsRemoved(const QList<QDBusObjectPath> &cells);
    void cellRegisteredChanged(bool registered);
    void cellPropertyChanged(const QString &name, const QDBusVariant &value);

private:
    void detectModemAsync();
    void applySelectedModem(int index);
    void readSimLabelAsync(int index);
    void clearCellularData();
    void disconnectCellInfoSignals();
    void disconnectWatchedCells();
    QString baseSimLabel(int index) const;

    void connectCellInfoSignals();
    void subscribeCellInfoAsync();
    void readRegistrationAsync();
    void readImsAsync();
    void readCellAsync(const QString &path);
    void readCellTypeAsync(const QString &path);
    void readCellPropertiesAsync(const QString &path);
    void watchCell(const QString &path);
    void updateTechnologyDisplay();
    void updateLteCellIdentity(quint32 ci);

    void readMobileIpAsync();
    void parseMobileAddresses(const QString &output);

    void readWifiAsync();
    void readWifiInterfaceAsync(const QString &iw, const QString &ip,
                                const QString &dnsTool, const QString &iface);
    void finishRefreshPart();
    void parseWifiLink(const QString &output);
    void parseWifiAddress(const QString &output);
    void parseWifiRoute(const QString &output);
    void parseWifiRoute6(const QString &output);
    void parseWifiDns(const QString &output);
    QString detectWifiInterface(const QString &output) const;
    QString findProgram(const QString &name) const;

    static QString lteBandFromEarfcn(int earfcn);
    static int wifiChannelFromFrequency(int mhz);

    QDBusConnection m_bus;
    OfonoModemList m_modems;
    QStringList m_simLabels;
    int m_selectedSim = 0;

    QString m_modemPath;
    QStringList m_modemInterfaces;
    bool m_detectingModem = false;
    bool m_cellInfoSignalsConnected = false;

    QString m_servingCellPath;
    QString m_servingCellType;
    QSet<QString> m_watchedCells;

    bool m_refreshing = false;
    int m_pendingRefreshParts = 0;
    QStringList m_refreshWarnings;

    QString m_status = QStringLiteral("Starting…");

    QString m_operatorName = QStringLiteral("—");
    QString m_mccMnc = QStringLiteral("—");
    QString m_registrationTechnology;
    QString m_technology = QStringLiteral("—");
    bool m_nrActive = false;
    QString m_band = QStringLiteral("—");
    QString m_bandLabel = QStringLiteral("Band");
    QString m_earfcn = QStringLiteral("—");
    QString m_earfcnLabel = QStringLiteral("EARFCN");
    QString m_nrBand = QStringLiteral("—");
    QString m_cellId = QStringLiteral("—");
    QString m_eNbId = QStringLiteral("—");
    QString m_localCellId = QStringLiteral("—");
    QString m_tac = QStringLiteral("—");
    QString m_pci = QStringLiteral("—");
    QString m_rsrp = QStringLiteral("—");
    QString m_rsrq = QStringLiteral("—");
    QString m_sinr = QStringLiteral("—");
    QString m_cqi = QStringLiteral("—");
    QString m_timingAdvance = QStringLiteral("—");
    QString m_ims = QStringLiteral("—");
    QVariantList m_mobileIpEntries;

    QString m_wifiInterface = QStringLiteral("—");
    QString m_wifiSsid = QStringLiteral("—");
    QString m_wifiBssid = QStringLiteral("—");
    QString m_wifiBand = QStringLiteral("—");
    QString m_wifiChannel = QStringLiteral("—");
    QString m_wifiFrequency = QStringLiteral("—");
    QString m_wifiRssi = QStringLiteral("—");
    QString m_wifiRxRate = QStringLiteral("—");
    QString m_wifiTxRate = QStringLiteral("—");
    QString m_wifiIp = QStringLiteral("—");
    QString m_wifiIpv6 = QStringLiteral("—");
    QString m_wifiGateway = QStringLiteral("—");
    QString m_wifiGateway6 = QStringLiteral("—");
    QString m_wifiDns = QStringLiteral("—");
    QString m_wifiStatus = QStringLiteral("Čekám na načtení");
};
