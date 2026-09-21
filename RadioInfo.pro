TARGET = harbour-radioinfo

QT += core qml quick dbus
CONFIG += sailfishapp c++11

SOURCES += \
    src/main.cpp \
    src/networkinfo.cpp \
    src/languagemanager.cpp

HEADERS += \
    src/networkinfo.h \
    src/languagemanager.h \
    src/version.h

DISTFILES += \
    qml/harbour-radioinfo.qml \
    qml/pages/MainPage.qml \
    qml/cover/CoverPage.qml \
    rpm/harbour-radioinfo.spec \
    harbour-radioinfo.desktop \
    harbour-radioinfo.png \
    README.md \
    LICENSE \
    translations/harbour-radioinfo-en.ts \
    translations/harbour-radioinfo-cs.ts

CONFIG += sailfishapp_i18n

TRANSLATIONS += \
    translations/harbour-radioinfo-en.ts \
    translations/harbour-radioinfo-cs.ts

