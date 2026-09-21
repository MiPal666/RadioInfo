#include "networkinfo.h"

#include <QDateTime>
#include <QFileInfo>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <algorithm>

QDBusArgument &operator<<(QDBusArgument &argument, const OfonoModem &modem)
{
    argument.beginStructure();
    argument << modem.path << modem.properties;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, OfonoModem &modem)
{
    argument.beginStructure();
    argument >> modem.path >> modem.properties;
    argument.endStructure();
    return argument;
}

NetworkInfo::NetworkInfo(QObject *parent)
    : QObject(parent),
      m_bus(QDBusConnection::systemBus())
{
    qDBusRegisterMetaType<OfonoModem>();
    qDBusRegisterMetaType<OfonoModemList>();

    if (!m_bus.isConnected()) {
        m_status = tr("Error: system D-Bus is not available");
        return;
    }

    m_status = tr("Searching for modem…");
    detectModemAsync();
}

void NetworkInfo::detectModemAsync()
{
    if (m_detectingModem)
        return;

    m_detectingModem = true;

    QDBusInterface *manager = new QDBusInterface(
        QStringLiteral("org.ofono"),
        QStringLiteral("/"),
        QStringLiteral("org.ofono.Manager"),
        m_bus,
        this);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
        manager->asyncCall(QStringLiteral("GetModems")), this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, watcher, manager](QDBusPendingCallWatcher *) {
        QDBusPendingReply<OfonoModemList> reply = *watcher;
        m_detectingModem = false;

        if (reply.isError()) {
            m_status = tr("oFono modem not found: %1")
                    .arg(reply.error().message());
            emit dataChanged();
            watcher->deleteLater();
            manager->deleteLater();
            return;
        }

        m_modems = reply.value();

        // Keep /ril_0, /ril_1 ... in deterministic physical-slot order.
        std::sort(m_modems.begin(), m_modems.end(),
                  [](const OfonoModem &a, const OfonoModem &b) {
            return a.path.path() < b.path.path();
        });

        if (m_modems.isEmpty()) {
            m_status = tr("No oFono modem was found");
            emit dataChanged();
            emit simListChanged();
            watcher->deleteLater();
            manager->deleteLater();
            return;
        }

        m_simLabels.clear();
        for (int i = 0; i < m_modems.size(); ++i)
            m_simLabels << baseSimLabel(i);

        emit simListChanged();

        // Prefer the first online/powered modem which exposes network registration.
        int preferred = 0;
        for (int i = 0; i < m_modems.size(); ++i) {
            const QVariantMap p = m_modems.at(i).properties;
            const QStringList interfaces =
                    p.value(QStringLiteral("Interfaces")).toStringList();
            if (interfaces.contains(QStringLiteral("org.ofono.NetworkRegistration")) &&
                    p.value(QStringLiteral("Online")).toBool() &&
                    p.value(QStringLiteral("Powered")).toBool()) {
                preferred = i;
                break;
            }
        }

        // Resolve operator/SPN labels for both slots independently.
        for (int i = 0; i < m_modems.size(); ++i)
            readSimLabelAsync(i);

        applySelectedModem(preferred);

        watcher->deleteLater();
        manager->deleteLater();
    });
}

QString NetworkInfo::baseSimLabel(int index) const
{
    if (index < 0 || index >= m_modems.size())
        return QStringLiteral("SIM %1").arg(index + 1);

    const QString path = m_modems.at(index).path.path();
    QRegularExpression re(QStringLiteral("/ril_(\\d+)$"));
    QRegularExpressionMatch match = re.match(path);
    if (match.hasMatch())
        return QStringLiteral("SIM %1").arg(match.captured(1).toInt() + 1);

    return QStringLiteral("SIM %1").arg(index + 1);
}

void NetworkInfo::readSimLabelAsync(int index)
{
    if (index < 0 || index >= m_modems.size())
        return;

    const OfonoModem modem = m_modems.at(index);
    const QString path = modem.path.path();
    const QStringList interfaces =
            modem.properties.value(QStringLiteral("Interfaces")).toStringList();
    const QString base = baseSimLabel(index);

    if (interfaces.contains(QStringLiteral("org.ofono.NetworkRegistration"))) {
        QDBusInterface *iface = new QDBusInterface(
            QStringLiteral("org.ofono"),
            path,
            QStringLiteral("org.ofono.NetworkRegistration"),
            m_bus,
            this);

        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            iface->asyncCall(QStringLiteral("GetProperties")), this);

        connect(watcher, &QDBusPendingCallWatcher::finished, this,
                [this, watcher, iface, index, base, path, interfaces](QDBusPendingCallWatcher *) {
            QDBusPendingReply<QVariantMap> reply = *watcher;
            if (!reply.isError()) {
                const QVariantMap p = reply.value();
                QString name = p.value(QStringLiteral("Name")).toString().trimmed();
                if (name.isEmpty()) {
                    const QString mcc = p.value(QStringLiteral("MobileCountryCode")).toString();
                    const QString mnc = p.value(QStringLiteral("MobileNetworkCode")).toString();
                    if (!mcc.isEmpty() && !mnc.isEmpty())
                        name = QStringLiteral("%1/%2").arg(mcc, mnc);
                }

                if (!name.isEmpty() && index < m_simLabels.size()) {
                    m_simLabels[index] = base + QStringLiteral(" — ") + name;
                    emit simListChanged();
                }
            } else if (interfaces.contains(QStringLiteral("org.nemomobile.ofono.SimInfo"))) {
                // Registration can be temporarily unavailable; fall back to SIM SPN.
                QDBusInterface *sim = new QDBusInterface(
                    QStringLiteral("org.ofono"),
                    path,
                    QStringLiteral("org.nemomobile.ofono.SimInfo"),
                    m_bus,
                    this);
                QDBusPendingCallWatcher *sw = new QDBusPendingCallWatcher(
                    sim->asyncCall(QStringLiteral("GetServiceProviderName")), this);
                connect(sw, &QDBusPendingCallWatcher::finished, this,
                        [this, sw, sim, index, base](QDBusPendingCallWatcher *) {
                    QDBusPendingReply<QString> sr = *sw;
                    if (!sr.isError() && !sr.value().trimmed().isEmpty() &&
                            index < m_simLabels.size()) {
                        m_simLabels[index] = base + QStringLiteral(" — ") + sr.value().trimmed();
                        emit simListChanged();
                    }
                    sw->deleteLater();
                    sim->deleteLater();
                });
            }

            watcher->deleteLater();
            iface->deleteLater();
        });
        return;
    }

    if (interfaces.contains(QStringLiteral("org.nemomobile.ofono.SimInfo"))) {
        QDBusInterface *iface = new QDBusInterface(
            QStringLiteral("org.ofono"),
            path,
            QStringLiteral("org.nemomobile.ofono.SimInfo"),
            m_bus,
            this);

        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            iface->asyncCall(QStringLiteral("GetServiceProviderName")), this);

        connect(watcher, &QDBusPendingCallWatcher::finished, this,
                [this, watcher, iface, index, base](QDBusPendingCallWatcher *) {
            QDBusPendingReply<QString> reply = *watcher;
            if (!reply.isError() && !reply.value().trimmed().isEmpty() &&
                    index < m_simLabels.size()) {
                m_simLabels[index] = base + QStringLiteral(" — ") + reply.value().trimmed();
                emit simListChanged();
            }
            watcher->deleteLater();
            iface->deleteLater();
        });
    }
}

void NetworkInfo::selectSim(int index)
{
    if (index < 0 || index >= m_modems.size() || index == m_selectedSim)
        return;

    if (m_refreshing)
        return;

    applySelectedModem(index);
}

void NetworkInfo::applySelectedModem(int index)
{
    if (index < 0 || index >= m_modems.size())
        return;

    disconnectCellInfoSignals();
    disconnectWatchedCells();

    m_selectedSim = index;
    m_modemPath = m_modems.at(index).path.path();
    m_modemInterfaces = m_modems.at(index).properties.value(
                QStringLiteral("Interfaces")).toStringList();

    clearCellularData();

    connectCellInfoSignals();
    subscribeCellInfoAsync();

    emit selectedSimChanged();
    emit dataChanged();

    refresh();
}

void NetworkInfo::disconnectCellInfoSignals()
{
    if (!m_cellInfoSignalsConnected || m_modemPath.isEmpty())
        return;

    m_bus.disconnect(
        QStringLiteral("org.ofono"),
        m_modemPath,
        QStringLiteral("org.nemomobile.ofono.CellInfo"),
        QStringLiteral("CellsAdded"),
        this,
        SLOT(cellsAdded(QList<QDBusObjectPath>)));

    m_bus.disconnect(
        QStringLiteral("org.ofono"),
        m_modemPath,
        QStringLiteral("org.nemomobile.ofono.CellInfo"),
        QStringLiteral("CellsRemoved"),
        this,
        SLOT(cellsRemoved(QList<QDBusObjectPath>)));

    m_cellInfoSignalsConnected = false;
}

void NetworkInfo::disconnectWatchedCells()
{
    for (const QString &path : m_watchedCells) {
        m_bus.disconnect(
            QStringLiteral("org.ofono"),
            path,
            QStringLiteral("org.nemomobile.ofono.Cell"),
            QStringLiteral("RegisteredChanged"),
            this,
            SLOT(cellRegisteredChanged(bool)));

        m_bus.disconnect(
            QStringLiteral("org.ofono"),
            path,
            QStringLiteral("org.nemomobile.ofono.Cell"),
            QStringLiteral("PropertyChanged"),
            this,
            SLOT(cellPropertyChanged(QString,QDBusVariant)));
    }

    m_watchedCells.clear();
}

void NetworkInfo::clearCellularData()
{
    m_servingCellPath.clear();
    m_servingCellType.clear();

    m_operatorName = QStringLiteral("—");
    m_mccMnc = QStringLiteral("—");
    m_registrationTechnology.clear();
    m_technology = QStringLiteral("—");
    m_nrActive = false;

    m_band = QStringLiteral("—");
    m_bandLabel = tr("Band");
    m_earfcn = QStringLiteral("—");
    m_earfcnLabel = QStringLiteral("EARFCN");
    m_nrBand = QStringLiteral("—");

    m_cellId = QStringLiteral("—");
    m_eNbId = QStringLiteral("—");
    m_localCellId = QStringLiteral("—");
    m_tac = QStringLiteral("—");
    m_pci = QStringLiteral("—");
    m_rsrp = QStringLiteral("—");
    m_rsrq = QStringLiteral("—");
    m_sinr = QStringLiteral("—");
    m_cqi = QStringLiteral("—");
    m_timingAdvance = QStringLiteral("—");
    m_ims = QStringLiteral("—");
}

void NetworkInfo::connectCellInfoSignals()
{
    if (m_cellInfoSignalsConnected || m_modemPath.isEmpty())
        return;

    if (!m_modemInterfaces.contains(QStringLiteral("org.nemomobile.ofono.CellInfo")))
        return;

    const bool added = m_bus.connect(
        QStringLiteral("org.ofono"),
        m_modemPath,
        QStringLiteral("org.nemomobile.ofono.CellInfo"),
        QStringLiteral("CellsAdded"),
        this,
        SLOT(cellsAdded(QList<QDBusObjectPath>)));

    const bool removed = m_bus.connect(
        QStringLiteral("org.ofono"),
        m_modemPath,
        QStringLiteral("org.nemomobile.ofono.CellInfo"),
        QStringLiteral("CellsRemoved"),
        this,
        SLOT(cellsRemoved(QList<QDBusObjectPath>)));

    m_cellInfoSignalsConnected = added && removed;
}

void NetworkInfo::refresh()
{
    if (m_refreshing)
        return;

    if (m_modemPath.isEmpty()) {
        m_status = tr("Searching for modem…");
        emit dataChanged();
        detectModemAsync();
        return;
    }

    m_refreshing = true;
    m_pendingRefreshParts = 3; // registration, IMS, Wi-Fi
    m_refreshWarnings.clear();
    m_status = tr("Refreshing…");
    emit refreshingChanged();
    emit dataChanged();

    if (!m_servingCellPath.isEmpty()) {
        readCellTypeAsync(m_servingCellPath);
        readCellPropertiesAsync(m_servingCellPath);
    } else {
        subscribeCellInfoAsync();
    }

    if (m_modemInterfaces.contains(QStringLiteral("org.ofono.NetworkRegistration"))) {
        readRegistrationAsync();
    } else {
        m_operatorName = selectedSimLabel();
        m_technology = tr("Unavailable");
        m_refreshWarnings << tr("SIM is not registered on the network");
        emit dataChanged();
        finishRefreshPart();
    }

    readImsAsync();
    readWifiAsync();
}

void NetworkInfo::finishRefreshPart()
{
    if (m_pendingRefreshParts > 0)
        --m_pendingRefreshParts;

    if (m_pendingRefreshParts != 0)
        return;

    m_refreshing = false;
    const QString when = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    if (m_refreshWarnings.isEmpty())
        m_status = tr("Updated %1").arg(when);
    else
        m_status = tr("Updated %1 — %2")
                .arg(when, m_refreshWarnings.join(QStringLiteral("; ")));

    emit refreshingChanged();
    emit dataChanged();
}

void NetworkInfo::subscribeCellInfoAsync()
{
    if (m_modemPath.isEmpty() ||
            !m_modemInterfaces.contains(QStringLiteral("org.nemomobile.ofono.CellInfo")))
        return;

    QDBusInterface *iface = new QDBusInterface(
        QStringLiteral("org.ofono"),
        m_modemPath,
        QStringLiteral("org.nemomobile.ofono.CellInfo"),
        m_bus,
        this);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
        iface->asyncCall(QStringLiteral("GetCells")), this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, watcher, iface](QDBusPendingCallWatcher *) {
        QDBusPendingReply<QList<QDBusObjectPath>> reply = *watcher;
        if (!reply.isError()) {
            for (const QDBusObjectPath &cell : reply.value())
                readCellAsync(cell.path());
        }
        watcher->deleteLater();
        iface->deleteLater();
    });
}

void NetworkInfo::cellsAdded(const QList<QDBusObjectPath> &cells)
{
    for (const QDBusObjectPath &cell : cells)
        readCellAsync(cell.path());
}

void NetworkInfo::cellsRemoved(const QList<QDBusObjectPath> &cells)
{
    for (const QDBusObjectPath &cell : cells) {
        m_watchedCells.remove(cell.path());
        if (cell.path() == m_servingCellPath) {
            m_servingCellPath.clear();
            m_servingCellType.clear();
            updateTechnologyDisplay();
            m_status = tr("Serving cell removed, waiting for a new one…");
            emit dataChanged();
        }
    }
}

void NetworkInfo::readCellAsync(const QString &path)
{
    QDBusInterface *iface = new QDBusInterface(
        QStringLiteral("org.ofono"),
        path,
        QStringLiteral("org.nemomobile.ofono.Cell"),
        m_bus,
        this);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
        iface->asyncCall(QStringLiteral("GetRegistered")), this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, watcher, iface, path](QDBusPendingCallWatcher *) {
        QDBusPendingReply<bool> reply = *watcher;
        if (!reply.isError() && reply.value()) {
            m_servingCellPath = path;
            watchCell(path);
            readCellTypeAsync(path);
            readCellPropertiesAsync(path);
        }
        watcher->deleteLater();
        iface->deleteLater();
    });
}

void NetworkInfo::readCellTypeAsync(const QString &path)
{
    QDBusInterface *iface = new QDBusInterface(
        QStringLiteral("org.ofono"),
        path,
        QStringLiteral("org.nemomobile.ofono.Cell"),
        m_bus,
        this);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
        iface->asyncCall(QStringLiteral("GetType")), this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, watcher, iface](QDBusPendingCallWatcher *) {
        QDBusPendingReply<QString> reply = *watcher;
        if (!reply.isError()) {
            m_servingCellType = reply.value().toLower();
            if (m_servingCellType == QStringLiteral("nr")) {
                // Do not carry an old LTE band/channel into a true NR serving cell.
                m_band = QStringLiteral("—");
                m_earfcn = QStringLiteral("—");
            }
            updateTechnologyDisplay();
            emit dataChanged();
        }
        watcher->deleteLater();
        iface->deleteLater();
    });
}

void NetworkInfo::readCellPropertiesAsync(const QString &path)
{
    QDBusInterface *iface = new QDBusInterface(
        QStringLiteral("org.ofono"),
        path,
        QStringLiteral("org.nemomobile.ofono.Cell"),
        m_bus,
        this);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
        iface->asyncCall(QStringLiteral("GetProperties")), this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, watcher, iface](QDBusPendingCallWatcher *) {
        QDBusPendingReply<QVariantMap> reply = *watcher;
        if (!reply.isError()) {
            const QVariantMap p = reply.value();

            if (m_mccMnc == QStringLiteral("—") &&
                    p.contains(QStringLiteral("mcc")) && p.contains(QStringLiteral("mnc")))
                m_mccMnc = QStringLiteral("%1 / %2")
                        .arg(p.value(QStringLiteral("mcc")).toInt())
                        .arg(p.value(QStringLiteral("mnc")).toInt());

            if (p.contains(QStringLiteral("ci")))
                updateLteCellIdentity(p.value(QStringLiteral("ci")).toUInt());
            if (p.contains(QStringLiteral("pci")))
                m_pci = QString::number(p.value(QStringLiteral("pci")).toInt());
            if (p.contains(QStringLiteral("tac")))
                m_tac = QString::number(p.value(QStringLiteral("tac")).toInt());

            // Sailfish CellInfo currently exposes the LTE anchor EARFCN on the
            // tested MediaTek stack even while NetworkRegistration reports NR.
            if (p.contains(QStringLiteral("earfcn"))) {
                const int e = p.value(QStringLiteral("earfcn")).toInt();
                m_earfcn = QString::number(e);
                m_band = lteBandFromEarfcn(e);
            }

            if (p.contains(QStringLiteral("rsrp"))) {
                const int v = p.value(QStringLiteral("rsrp")).toInt();
                m_rsrp = QStringLiteral("-%1 dBm (raw %1)").arg(v);
            }
            if (p.contains(QStringLiteral("rsrq"))) {
                const int v = p.value(QStringLiteral("rsrq")).toInt();
                m_rsrq = QStringLiteral("-%1 dB (raw %1)").arg(v);
            }
            if (p.contains(QStringLiteral("rssnr"))) {
                const int v = p.value(QStringLiteral("rssnr")).toInt();
                m_sinr = QStringLiteral("%1 dB (raw %2)")
                        .arg(v / 10.0, 0, 'f', 1)
                        .arg(v);
            }
            if (p.contains(QStringLiteral("cqi")))
                m_cqi = QString::number(p.value(QStringLiteral("cqi")).toInt());
            if (p.contains(QStringLiteral("timingAdvance")))
                m_timingAdvance = QString::number(
                            p.value(QStringLiteral("timingAdvance")).toInt())
                        + QStringLiteral(" (raw)");

            updateTechnologyDisplay();
            emit dataChanged();
        }
        watcher->deleteLater();
        iface->deleteLater();
    });
}

void NetworkInfo::watchCell(const QString &path)
{
    if (m_watchedCells.contains(path))
        return;

    m_watchedCells.insert(path);

    m_bus.connect(
        QStringLiteral("org.ofono"),
        path,
        QStringLiteral("org.nemomobile.ofono.Cell"),
        QStringLiteral("RegisteredChanged"),
        this,
        SLOT(cellRegisteredChanged(bool)));

    m_bus.connect(
        QStringLiteral("org.ofono"),
        path,
        QStringLiteral("org.nemomobile.ofono.Cell"),
        QStringLiteral("PropertyChanged"),
        this,
        SLOT(cellPropertyChanged(QString,QDBusVariant)));
}

void NetworkInfo::cellRegisteredChanged(bool registered)
{
    if (registered)
        subscribeCellInfoAsync();
}

void NetworkInfo::cellPropertyChanged(const QString &name, const QDBusVariant &value)
{
    if (name == QStringLiteral("rsrp")) {
        const int v = value.variant().toInt();
        m_rsrp = QStringLiteral("-%1 dBm (raw %1)").arg(v);
    } else if (name == QStringLiteral("rsrq")) {
        const int v = value.variant().toInt();
        m_rsrq = QStringLiteral("-%1 dB (raw %1)").arg(v);
    } else if (name == QStringLiteral("rssnr")) {
        const int v = value.variant().toInt();
        m_sinr = QStringLiteral("%1 dB (raw %2)")
                .arg(v / 10.0, 0, 'f', 1)
                .arg(v);
    } else if (name == QStringLiteral("pci")) {
        m_pci = QString::number(value.variant().toInt());
    } else if (name == QStringLiteral("tac")) {
        m_tac = QString::number(value.variant().toInt());
    } else if (name == QStringLiteral("ci")) {
        updateLteCellIdentity(value.variant().toUInt());
    } else if (name == QStringLiteral("earfcn")) {
        const int e = value.variant().toInt();
        m_earfcn = QString::number(e);
        m_band = lteBandFromEarfcn(e);
    } else if (name == QStringLiteral("cqi")) {
        m_cqi = QString::number(value.variant().toInt());
    } else if (name == QStringLiteral("timingAdvance")) {
        m_timingAdvance = QString::number(value.variant().toInt()) + QStringLiteral(" (raw)");
    }

    updateTechnologyDisplay();
    emit dataChanged();
}

void NetworkInfo::updateLteCellIdentity(quint32 ci)
{
    // LTE E-UTRAN Cell Identifier (ECI) is 28 bits:
    // upper 20 bits = eNodeB ID, lower 8 bits = local Cell ID.
    // 0x0fffffff is commonly used by telephony stacks as "unknown".
    if (ci == 0x0fffffffU) {
        m_cellId = QStringLiteral("—");
        m_eNbId = QStringLiteral("—");
        m_localCellId = QStringLiteral("—");
        return;
    }

    m_cellId = QString::number(ci);
    m_eNbId = QString::number(ci >> 8);
    m_localCellId = QString::number(ci & 0xffU);
}

void NetworkInfo::updateTechnologyDisplay()
{
    const QString raw = m_registrationTechnology.toLower();
    m_nrActive = (raw == QStringLiteral("nr") || raw == QStringLiteral("5g"));

    if (m_nrActive) {
        if (m_servingCellType == QStringLiteral("lte"))
            m_technology = QStringLiteral("5G NSA");
        else if (m_servingCellType == QStringLiteral("nr"))
            m_technology = QStringLiteral("5G SA");
        else
            m_technology = QStringLiteral("5G NR");

        if (m_servingCellType == QStringLiteral("nr")) {
            m_bandLabel = tr("NR band");
            m_earfcnLabel = QStringLiteral("NR-ARFCN");
        } else {
            m_bandLabel = tr("LTE anchor");
            m_earfcnLabel = tr("LTE EARFCN");
        }
        // Current oFono/CellInfo on the tested device does not expose a
        // separate NR cell/NRARFCN, so never invent an NR band.
        m_nrBand = tr("Unavailable");
        return;
    }

    m_nrBand = QStringLiteral("—");
    m_bandLabel = tr("Band");
    m_earfcnLabel = QStringLiteral("EARFCN");

    if (raw == QStringLiteral("lte"))
        m_technology = QStringLiteral("LTE");
    else if (raw == QStringLiteral("umts"))
        m_technology = QStringLiteral("UMTS");
    else if (raw == QStringLiteral("gsm"))
        m_technology = QStringLiteral("GSM");
    else if (!raw.isEmpty())
        m_technology = raw.toUpper();
    else
        m_technology = QStringLiteral("—");
}

void NetworkInfo::retranslate()
{
    // Recreate labels which are stored as strings in the backend.
    updateTechnologyDisplay();
    emit dataChanged();

    // Re-read user-facing status/IMS/Wi-Fi texts in the newly selected
    // language. Radio values continue to be updated by the CellInfo signals.
    if (!m_refreshing)
        refresh();
}

void NetworkInfo::readRegistrationAsync()
{
    QDBusInterface *iface = new QDBusInterface(
        QStringLiteral("org.ofono"),
        m_modemPath,
        QStringLiteral("org.ofono.NetworkRegistration"),
        m_bus,
        this);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
        iface->asyncCall(QStringLiteral("GetProperties")), this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, watcher, iface](QDBusPendingCallWatcher *) {
        QDBusPendingReply<QVariantMap> reply = *watcher;
        if (!reply.isError()) {
            const QVariantMap p = reply.value();

            const QString mcc = p.value(QStringLiteral("MobileCountryCode")).toString();
            const QString mnc = p.value(QStringLiteral("MobileNetworkCode")).toString();

            if (!mcc.isEmpty() && !mnc.isEmpty())
                m_mccMnc = mcc + QStringLiteral(" / ") + mnc;

            const QString name = p.value(QStringLiteral("Name")).toString().trimmed();
            if (!name.isEmpty())
                m_operatorName = name;
            else if (!mcc.isEmpty() && !mnc.isEmpty())
                m_operatorName = QStringLiteral("%1/%2").arg(mcc, mnc);
            else
                m_operatorName = QStringLiteral("—");

            m_registrationTechnology = p.value(
                        QStringLiteral("Technology")).toString().toLower();
            updateTechnologyDisplay();

            if (p.contains(QStringLiteral("CellId")) && m_cellId == QStringLiteral("—"))
                updateLteCellIdentity(p.value(QStringLiteral("CellId")).toUInt());

            emit dataChanged();
        } else {
            m_refreshWarnings << tr("oFono registration unavailable");
        }
        watcher->deleteLater();
        iface->deleteLater();
        finishRefreshPart();
    });
}

void NetworkInfo::readImsAsync()
{
    if (!m_modemInterfaces.contains(QStringLiteral("org.ofono.IpMultimediaSystem"))) {
        m_ims = tr("Unavailable");
        emit dataChanged();
        finishRefreshPart();
        return;
    }

    QDBusInterface *iface = new QDBusInterface(
        QStringLiteral("org.ofono"),
        m_modemPath,
        QStringLiteral("org.ofono.IpMultimediaSystem"),
        m_bus,
        this);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
        iface->asyncCall(QStringLiteral("GetProperties")), this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, watcher, iface](QDBusPendingCallWatcher *) {
        QDBusPendingReply<QVariantMap> reply = *watcher;
        if (!reply.isError()) {
            const QVariantMap p = reply.value();
            if (p.value(QStringLiteral("Registered")).toBool()) {
                QStringList caps;
                if (p.value(QStringLiteral("VoiceCapable")).toBool())
                    caps << tr("voice");
                if (p.value(QStringLiteral("SmsCapable")).toBool())
                    caps << QStringLiteral("SMS");
                m_ims = tr("Registered");
                if (!caps.isEmpty())
                    m_ims += QStringLiteral(" (%1)").arg(caps.join(QStringLiteral(", ")));
            } else {
                m_ims = tr("Not registered");
            }
            emit dataChanged();
        } else {
            m_ims = tr("Unavailable");
            m_refreshWarnings << tr("IMS unavailable");
        }
        watcher->deleteLater();
        iface->deleteLater();
        finishRefreshPart();
    });
}

QString NetworkInfo::findProgram(const QString &name) const
{
    QString program = QStandardPaths::findExecutable(name);
    if (!program.isEmpty())
        return program;

    const QStringList candidates = QStringList()
            << QStringLiteral("/usr/bin/") + name
            << QStringLiteral("/usr/sbin/") + name
            << QStringLiteral("/bin/") + name
            << QStringLiteral("/sbin/") + name;

    for (const QString &candidate : candidates) {
        if (QFileInfo(candidate).isExecutable())
            return candidate;
    }
    return QString();
}

QString NetworkInfo::detectWifiInterface(const QString &output) const
{
    QString current;
    QString firstManaged;
    QString connectedManaged;
    bool managed = false;
    bool hasSsid = false;

    auto finishCurrent = [&]() {
        if (current.isEmpty() || !managed)
            return;
        if (firstManaged.isEmpty())
            firstManaged = current;
        if (hasSsid && connectedManaged.isEmpty())
            connectedManaged = current;
    };

    const QStringList lines = output.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        const QString t = line.trimmed();
        if (t.startsWith(QStringLiteral("Interface "))) {
            finishCurrent();
            current = t.mid(QStringLiteral("Interface ").size()).trimmed();
            managed = false;
            hasSsid = false;
        } else if (t == QStringLiteral("type managed")) {
            managed = true;
        } else if (t.startsWith(QStringLiteral("ssid "))) {
            hasSsid = true;
        }
    }
    finishCurrent();

    return !connectedManaged.isEmpty() ? connectedManaged : firstManaged;
}

void NetworkInfo::readWifiAsync()
{
    m_wifiInterface = QStringLiteral("—");
    m_wifiSsid = m_wifiBssid = m_wifiBand = m_wifiChannel = QStringLiteral("—");
    m_wifiFrequency = m_wifiRssi = m_wifiRxRate = m_wifiTxRate = QStringLiteral("—");
    m_wifiIp = m_wifiGateway = m_wifiDns = QStringLiteral("—");
    m_wifiStatus = tr("Loading…");
    emit dataChanged();

    const QString iw = findProgram(QStringLiteral("iw"));
    const QString ip = findProgram(QStringLiteral("ip"));

    if (iw.isEmpty() || ip.isEmpty()) {
        QStringList missing;
        if (iw.isEmpty()) missing << QStringLiteral("iw");
        if (ip.isEmpty()) missing << QStringLiteral("ip");
        m_wifiStatus = tr("Missing program: %1").arg(missing.join(QStringLiteral(", ")));
        m_refreshWarnings << QStringLiteral("Wi-Fi: %1").arg(m_wifiStatus);
        emit dataChanged();
        finishRefreshPart();
        return;
    }

    QString dnsTool = findProgram(QStringLiteral("resolvectl"));
    if (dnsTool.isEmpty())
        dnsTool = findProgram(QStringLiteral("systemd-resolve"));

    QProcess *dev = new QProcess(this);
    connect(dev, static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished), this,
            [this, dev, iw, ip, dnsTool](int code, QProcess::ExitStatus) {
        const QString out = QString::fromUtf8(dev->readAllStandardOutput());
        const QString err = QString::fromUtf8(dev->readAllStandardError()).trimmed();
        dev->deleteLater();

        if (code != 0) {
            m_wifiStatus = QStringLiteral("iw dev: %1")
                    .arg(err.isEmpty() ? tr("no data") : err);
            m_refreshWarnings << QStringLiteral("Wi-Fi: %1").arg(m_wifiStatus);
            emit dataChanged();
            finishRefreshPart();
            return;
        }

        const QString iface = detectWifiInterface(out);
        if (iface.isEmpty()) {
            m_wifiStatus = tr("Wi-Fi interface not found");
            emit dataChanged();
            finishRefreshPart();
            return;
        }

        m_wifiInterface = iface;
        emit dataChanged();
        readWifiInterfaceAsync(iw, ip, dnsTool, iface);
    });
    connect(dev, &QProcess::errorOccurred, this,
            [this, dev](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        m_wifiStatus = tr("iw: cannot start (%1)").arg(dev->errorString());
        m_refreshWarnings << QStringLiteral("Wi-Fi: %1").arg(m_wifiStatus);
        dev->deleteLater();
        emit dataChanged();
        finishRefreshPart();
    });
    dev->start(iw, QStringList() << QStringLiteral("dev"));
}

void NetworkInfo::readWifiInterfaceAsync(const QString &iw, const QString &ip,
                                         const QString &dnsTool, const QString &iface)
{
    struct SharedState {
        int pending = 3;
        bool disconnected = false;
        QStringList errors;
    };

    SharedState *state = new SharedState;
    if (!dnsTool.isEmpty())
        ++state->pending;

    auto done = [this, state]() {
        --state->pending;
        if (state->pending != 0)
            return;

        if (state->disconnected) {
            m_wifiStatus = tr("Disconnected");
        } else if (state->errors.isEmpty()) {
            m_wifiStatus = QStringLiteral("OK");
        } else {
            m_wifiStatus = state->errors.join(QStringLiteral("; "));
            m_refreshWarnings << QStringLiteral("Wi-Fi: %1").arg(m_wifiStatus);
        }

        delete state;
        emit dataChanged();
        finishRefreshPart();
    };

    QProcess *link = new QProcess(this);
    connect(link, static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished), this,
            [this, link, state, done](int code, QProcess::ExitStatus) {
        const QString out = QString::fromUtf8(link->readAllStandardOutput());
        const QString err = QString::fromUtf8(link->readAllStandardError()).trimmed();
        if (code == 0) {
            if (out.contains(QStringLiteral("Not connected"), Qt::CaseInsensitive))
                state->disconnected = true;
            else
                parseWifiLink(out);
        } else {
            state->errors << QStringLiteral("iw: %1")
                    .arg(err.isEmpty() ? tr("no data") : err);
        }
        link->deleteLater();
        done();
    });
    connect(link, &QProcess::errorOccurred, this,
            [this, link, state, done](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        state->errors << tr("iw: cannot start (%1)").arg(link->errorString());
        link->deleteLater();
        done();
    });
    link->start(iw, QStringList() << QStringLiteral("dev") << iface << QStringLiteral("link"));

    QProcess *addr = new QProcess(this);
    connect(addr, static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished), this,
            [this, addr, state, done](int code, QProcess::ExitStatus) {
        const QString out = QString::fromUtf8(addr->readAllStandardOutput());
        const QString err = QString::fromUtf8(addr->readAllStandardError()).trimmed();
        if (code == 0)
            parseWifiAddress(out);
        else
            state->errors << QStringLiteral("ip addr: %1")
                    .arg(err.isEmpty() ? tr("error") : err);
        addr->deleteLater();
        done();
    });
    connect(addr, &QProcess::errorOccurred, this,
            [this, addr, state, done](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        state->errors << tr("ip addr: cannot start (%1)").arg(addr->errorString());
        addr->deleteLater();
        done();
    });
    addr->start(ip, QStringList() << QStringLiteral("-4") << QStringLiteral("-o")
                                  << QStringLiteral("addr") << QStringLiteral("show")
                                  << QStringLiteral("dev") << iface);

    QProcess *route = new QProcess(this);
    connect(route, static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished), this,
            [this, route, state, done](int code, QProcess::ExitStatus) {
        const QString out = QString::fromUtf8(route->readAllStandardOutput());
        const QString err = QString::fromUtf8(route->readAllStandardError()).trimmed();
        if (code == 0)
            parseWifiRoute(out);
        else
            state->errors << QStringLiteral("ip route: %1")
                    .arg(err.isEmpty() ? tr("error") : err);
        route->deleteLater();
        done();
    });
    connect(route, &QProcess::errorOccurred, this,
            [this, route, state, done](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        state->errors << tr("ip route: cannot start (%1)").arg(route->errorString());
        route->deleteLater();
        done();
    });
    route->start(ip, QStringList() << QStringLiteral("route") << QStringLiteral("show")
                                   << QStringLiteral("default") << QStringLiteral("dev") << iface);

    if (!dnsTool.isEmpty()) {
        QProcess *dns = new QProcess(this);
        connect(dns, static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished), this,
                [this, dns, done](int code, QProcess::ExitStatus) {
            if (code == 0)
                parseWifiDns(QString::fromUtf8(dns->readAllStandardOutput()));
            dns->deleteLater();
            done();
        });
        connect(dns, &QProcess::errorOccurred, this,
                [dns, done](QProcess::ProcessError error) {
            if (error != QProcess::FailedToStart)
                return;
            dns->deleteLater();
            done();
        });

        if (QFileInfo(dnsTool).fileName() == QStringLiteral("resolvectl"))
            dns->start(dnsTool, QStringList() << QStringLiteral("dns") << iface);
        else
            dns->start(dnsTool, QStringList() << QStringLiteral("--status") << iface);
    }
}

void NetworkInfo::parseWifiLink(const QString &output)
{
    QRegularExpression re;
    QRegularExpressionMatch m;

    re.setPattern(QStringLiteral("Connected to\\s+([^\\s]+)"));
    m = re.match(output);
    if (m.hasMatch()) m_wifiBssid = m.captured(1);

    re.setPattern(QStringLiteral("\\bSSID:\\s*(.+)"));
    m = re.match(output);
    if (m.hasMatch()) m_wifiSsid = m.captured(1).trimmed();

    re.setPattern(QStringLiteral("\\bfreq:\\s*([0-9.]+)"));
    m = re.match(output);
    if (m.hasMatch()) {
        const int mhz = qRound(m.captured(1).toDouble());
        m_wifiFrequency = QStringLiteral("%1 MHz").arg(mhz);
        const int channel = wifiChannelFromFrequency(mhz);
        m_wifiChannel = channel > 0 ? QString::number(channel) : QStringLiteral("—");
        if (mhz >= 2400 && mhz < 2500) m_wifiBand = QStringLiteral("2,4 GHz");
        else if (mhz >= 4900 && mhz < 5925) m_wifiBand = QStringLiteral("5 GHz");
        else if (mhz >= 5925) m_wifiBand = QStringLiteral("6 GHz");
    }

    re.setPattern(QStringLiteral("\\bsignal:\\s*(-?[0-9.]+)\\s*dBm"));
    m = re.match(output);
    if (m.hasMatch()) m_wifiRssi = m.captured(1) + QStringLiteral(" dBm");

    re.setPattern(QStringLiteral("\\brx bitrate:\\s*([^\\n]+)"));
    m = re.match(output);
    if (m.hasMatch()) m_wifiRxRate = m.captured(1).trimmed();

    re.setPattern(QStringLiteral("\\btx bitrate:\\s*([^\\n]+)"));
    m = re.match(output);
    if (m.hasMatch()) m_wifiTxRate = m.captured(1).trimmed();
}

void NetworkInfo::parseWifiAddress(const QString &output)
{
    QRegularExpression re(QStringLiteral("\\binet\\s+([^\\s]+)"));
    const QRegularExpressionMatch m = re.match(output);
    if (m.hasMatch()) m_wifiIp = m.captured(1);
}

void NetworkInfo::parseWifiRoute(const QString &output)
{
    QRegularExpression re(QStringLiteral("\\bvia\\s+([^\\s]+)"));
    const QRegularExpressionMatch m = re.match(output);
    if (m.hasMatch()) m_wifiGateway = m.captured(1);
}

void NetworkInfo::parseWifiDns(const QString &output)
{
    QStringList servers;

    if (output.contains(QStringLiteral("DNS Servers:"))) {
        bool collecting = false;
        const QStringList lines = output.split(QLatin1Char('\n'));
        for (const QString &line : lines) {
            const QString trimmed = line.trimmed();
            if (trimmed.startsWith(QStringLiteral("DNS Servers:"))) {
                collecting = true;
                const QString first = trimmed.mid(QStringLiteral("DNS Servers:").size()).trimmed();
                if (!first.isEmpty())
                    servers << first;
                continue;
            }
            if (collecting) {
                if (trimmed.isEmpty() || trimmed.contains(QLatin1Char(':')))
                    break;
                servers << trimmed;
            }
        }
    } else {
        // resolvectl dns <iface>: "Link N (iface): address address ..."
        const int colon = output.indexOf(QStringLiteral("):"));
        QString values;
        if (colon >= 0)
            values = output.mid(colon + 2).trimmed();
        else {
            const int simpleColon = output.indexOf(QLatin1Char(':'));
            if (simpleColon >= 0)
                values = output.mid(simpleColon + 1).trimmed();
        }
        if (!values.isEmpty())
            servers = values.split(QRegularExpression(QStringLiteral("\\s+")), QString::SkipEmptyParts);
    }

    if (!servers.isEmpty())
        m_wifiDns = servers.join(QStringLiteral(", "));
}

QString NetworkInfo::lteBandFromEarfcn(int e)
{
    if (e >= 0 && e <= 599)       return QStringLiteral("B1");
    if (e >= 600 && e <= 1199)    return QStringLiteral("B2");
    if (e >= 1200 && e <= 1949)   return QStringLiteral("B3");
    if (e >= 1950 && e <= 2399)   return QStringLiteral("B4");
    if (e >= 2400 && e <= 2649)   return QStringLiteral("B5");
    if (e >= 2750 && e <= 3449)   return QStringLiteral("B7");
    if (e >= 3450 && e <= 3799)   return QStringLiteral("B8");
    if (e >= 6150 && e <= 6449)   return QStringLiteral("B20");
    if (e >= 9210 && e <= 9659)   return QStringLiteral("B28");
    return QStringLiteral("—");
}

int NetworkInfo::wifiChannelFromFrequency(int mhz)
{
    if (mhz == 2484) return 14;
    if (mhz >= 2412 && mhz <= 2472) return (mhz - 2407) / 5;
    if (mhz >= 5000 && mhz <= 5900) return (mhz - 5000) / 5;
    if (mhz >= 5955 && mhz <= 7115) return (mhz - 5950) / 5;
    return 0;
}
