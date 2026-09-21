#include <sailfishapp.h>
#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QTimer>

#include "networkinfo.h"
#include "languagemanager.h"
#include "version.h"

int main(int argc, char *argv[])
{
    QGuiApplication *app = SailfishApp::application(argc, argv);
    app->setApplicationName(QStringLiteral("RadioInfo"));
    app->setApplicationVersion(QStringLiteral(RADIOINFO_VERSION));

    LanguageManager languageManager(
        app,
        SailfishApp::pathTo("translations").toLocalFile());

    QQuickView *view = SailfishApp::createView();

    NetworkInfo networkInfo;
    view->rootContext()->setContextProperty("networkInfo", &networkInfo);
    view->rootContext()->setContextProperty("languageManager", &languageManager);
    view->rootContext()->setContextProperty("appVersion", app->applicationVersion());

    const QUrl mainQml = SailfishApp::pathTo("qml/harbour-radioinfo.qml");

    QObject::connect(&languageManager, &LanguageManager::languageChanged,
                     app, [view, mainQml, &networkInfo]() {
        // Do this on the next event-loop turn. The language switch is
        // initiated by a QML control which must not destroy itself mid-call.
        QTimer::singleShot(0, view, [view, mainQml, &networkInfo]() {
            networkInfo.retranslate();
            view->setSource(QUrl());
            view->setSource(mainQml);
        });
    });

    view->setSource(mainQml);
    view->show();

    return app->exec();
}
