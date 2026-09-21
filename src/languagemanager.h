#pragma once

#include <QObject>
#include <QTranslator>

class QGuiApplication;

class LanguageManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString currentCode READ currentCode NOTIFY currentIndexChanged)

public:
    explicit LanguageManager(QGuiApplication *app,
                             const QString &translationDir,
                             QObject *parent = nullptr);

    int currentIndex() const { return m_currentIndex; }
    QString currentCode() const;

    Q_INVOKABLE void setCurrentIndex(int index);

signals:
    void currentIndexChanged();
    void languageChanged();

private:
    QString effectiveLanguageCode() const;
    QString codeForIndex(int index) const;
    int indexForCode(const QString &code) const;
    void applyLanguage();

    QGuiApplication *m_app = nullptr;
    QString m_translationDir;
    QString m_settingsPath;
    QString m_requestedCode;
    int m_currentIndex = 0;
    QTranslator m_translator;
};
