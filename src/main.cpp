#include <sailfishapp.h>
#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QLocale>
#include <QTranslator>

#include "networkinfo.h"
#include "version.h"

int main(int argc, char *argv[])
{
    QGuiApplication *app = SailfishApp::application(argc, argv);
    app->setApplicationName(QStringLiteral("FelisRadioInfo"));
    app->setApplicationVersion(QStringLiteral(RADIOINFO_VERSION));

    // English strings are the source/fallback language.  Load a translation
    // catalogue matching the Sailfish system language when one is available.
    QTranslator translator;
    const QString language = QLocale::system().name().section(QLatin1Char('_'), 0, 0).toLower();
    const QString translationDir = SailfishApp::pathTo("translations").toLocalFile();
    if (language != QStringLiteral("en") &&
            translator.load(QStringLiteral("harbour-radioinfo-") + language,
                            translationDir)) {
        app->installTranslator(&translator);
    }

    QQuickView *view = SailfishApp::createView();

    NetworkInfo networkInfo;
    view->rootContext()->setContextProperty("networkInfo", &networkInfo);
    view->rootContext()->setContextProperty("appVersion", app->applicationVersion());

    view->setSource(SailfishApp::pathTo("qml/harbour-radioinfo.qml"));
    view->show();

    return app->exec();
}
