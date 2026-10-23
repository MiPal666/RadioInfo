# FelisRadioInfo for Sailfish OS

FelisRadioInfo is a small native Sailfish OS diagnostic application for displaying
cellular and Wi-Fi connection details without telemetry, accounts or external
network services.


## Dual-SIM

FelisRadioInfo enumerates all oFono modems (`/ril_0`, `/ril_1`, ...), orders them by
slot, and allows switching the cellular diagnostics between SIMs. Operator
labels are resolved independently from network registration or the SIM service
provider name. Cellular state and CellInfo subscriptions follow the currently
selected SIM; Wi-Fi information is shared.

## Features

### Cellular

- operator name and MCC/MNC from oFono
- LTE / 5G NR state
- automatic distinction of `5G NSA` when oFono reports NR while CellInfo still
  exposes a registered LTE anchor cell
- LTE band derived from EARFCN
- Cell ID, LTE eNodeB ID (eNB), local Cell ID (CID), TAC and PCI
- RSRP, RSRQ, SINR, CQI and Timing Advance when exposed by CellInfo
- IMS registration and voice/SMS capability
- active mobile packet-data interfaces (`ccmni*`) with IPv4/IPv6 addresses
- persistent `org.nemomobile.ofono.CellInfo` subscription for live radio values

FelisRadioInfo does **not** invent NR information. On devices where oFono reports NR
but does not expose a separate NR CellInfo object/NRARFCN, the application shows
`NR band: Unavailable` and labels the visible LTE information as the LTE anchor.

### Wi-Fi

- automatically detects the managed Wi-Fi interface (no hard-coded `wlan0`)
- SSID / BSSID
- current Wi-Fi generation/standard from `wpa_supplicant` when available, with an `iw` PHY-rate fallback (Wi-Fi 4/5/6/7)
- band, channel, channel width and frequency; channel width is read from the current interface/link or associated BSS data
- RSSI
- current RX/TX bitrate as reported by `iw`
- RX/TX spatial streams (NSS) only when explicitly reported by the Wi-Fi driver; NSS is never estimated from link speed
- IPv4/IPv6 addresses and default gateways
- DNS servers when `resolvectl` or `systemd-resolve` is available

## Portability

The application does not hard-code a carrier, modem object path, Wi-Fi interface,
IP address, server or account. It discovers:

- the active oFono modem with `org.ofono.NetworkRegistration`
- the current operator from oFono
- the managed Wi-Fi interface from `iw dev`

Missing optional interfaces are shown as unavailable instead of being treated as
a fatal error. This is intended to make the application usable on other Sailfish
OS devices as well, although exact CellInfo properties are modem/vendor dependent.

LTE cell decomposition uses the standard ECI layout: `eNB = Cell ID >> 8` and `CID = Cell ID & 0xff`. For example, Cell ID `1357315` is eNB `5302`, CID `3`.

Tested on Jolla Phone 2026 with Sailfish OS 5.2.0.17. The project is currently
built against the SailfishOS 5.1.0.11 aarch64 SDK target.

## Sailjail note

The current Wi-Fi backend calls the local `iw` and `ip` utilities and can use `wpa_cli`
as an optional source for the negotiated Wi-Fi generation. Sailjail blocks the required
local tooling on the tested device, therefore the desktop file currently contains:

    [X-Sailjail]
    Sandboxing=Disabled

A future backend based entirely on ConnMan/nl80211 could remove this limitation.
This may matter for Harbour Store distribution; the current build is best suited
to direct installation, FelisCatus Apps or similar repositories.


## Languages

English is the source and fallback language. The application follows the Sailfish OS
system language automatically; there is no separate language selector inside the app.
Czech is included from the first public localized build. Additional translations can be
added as standard Qt `.ts` catalogues in `translations/`.

## User interface

The main view is split into two always-accessible tabs: **Mobile network** and **Wi-Fi**.
The tab switcher stays visible, so switching between radio and Wi-Fi diagnostics does not
require scrolling back through the other section.

The visible application name is **FelisRadioInfo**. The package, executable and desktop
file identifiers intentionally remain `harbour-radioinfo` to preserve upgrade compatibility
with existing installations.

## Build

With the Sailfish SDK configured for a suitable target:

    sfdk -c no-fix-version build

`no-fix-version` keeps the RPM version equal to the version in the spec file instead
of adding a Git branch/timestamp development suffix. The RPM is written below `RPMS/`.

## Privacy

FelisRadioInfo reads local system/network state only. It contains no telemetry and does
not send collected values anywhere.

## License

GPL-3.0-or-later. See `LICENSE`.
