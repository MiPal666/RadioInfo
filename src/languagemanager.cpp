#include "languagemanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QLocale>
#include <QSettings>
#include <QStandardPaths>

LanguageManager::LanguageManager(QGuiApplication *app,
                                 const QString &translationDir,
                                 QObject *parent)
    : QObject(parent),
      m_app(app),
      m_translationDir(translationDir)
{
    const QString configDir =
            QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    m_settingsPath = configDir + QStringLiteral("/radioinfo.conf");

    QSettings settings(m_settingsPath, QSettings::IniFormat);
    m_requestedCode = settings.value(
                QStringLiteral("ui/language"),
                QStringLiteral("system")).toString();

    m_currentIndex = indexForCode(m_requestedCode);
    if (m_currentIndex < 0) {
        m_currentIndex = 0;
        m_requestedCode = QStringLiteral("system");
    }

    applyLanguage();
}

QString LanguageManager::currentCode() const
{
    return m_requestedCode;
}

QString LanguageManager::codeForIndex(int index) const
{
    switch (index) {
    case 1:
        return QStringLiteral("en");
    case 2:
        return QStringLiteral("cs");
    default:
        return QStringLiteral("system");
    }
}

int LanguageManager::indexForCode(const QString &code) const
{
    if (code == QStringLiteral("en"))
        return 1;
    if (code == QStringLiteral("cs"))
        return 2;
    if (code == QStringLiteral("system"))
        return 0;
    return -1;
}

QString LanguageManager::effectiveLanguageCode() const
{
    if (m_requestedCode == QStringLiteral("en") ||
            m_requestedCode == QStringLiteral("cs"))
        return m_requestedCode;

    const QString systemLocale = QLocale::system().name().toLower();
    return systemLocale.startsWith(QStringLiteral("cs"))
            ? QStringLiteral("cs")
            : QStringLiteral("en");
}

void LanguageManager::applyLanguage()
{
    if (!m_app)
        return;

    m_app->removeTranslator(&m_translator);

    const QString language = effectiveLanguageCode();
    const QString catalog =
            QStringLiteral("harbour-radioinfo-") + language;

    // Load an explicit English catalogue too. This intentionally overrides
    // any automatically loaded system-locale catalogue when the user selects
    // English in the application.
    if (m_translator.load(catalog, m_translationDir))
        m_app->installTranslator(&m_translator);
}

void LanguageManager::setCurrentIndex(int index)
{
    if (index < 0 || index > 2 || index == m_currentIndex)
        return;

    m_currentIndex = index;
    m_requestedCode = codeForIndex(index);

    QSettings settings(m_settingsPath, QSettings::IniFormat);
    settings.setValue(QStringLiteral("ui/language"), m_requestedCode);
    settings.sync();

    applyLanguage();

    emit currentIndexChanged();
    emit languageChanged();
}
