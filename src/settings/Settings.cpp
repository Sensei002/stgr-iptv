#include "settings/Settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonValue>
#include <QSaveFile>

#include "core/AppPaths.h"
#include "core/version.h"

#ifdef Q_OS_WIN
#include <QSettings>
#endif

namespace {
const char kDefaultTheme[] = "dark";
constexpr int kDefaultBufferMs = 1500;
constexpr int kDefaultTimeoutSec = 10;
constexpr int kDefaultVolume = 60;
constexpr int kDefaultRetries = 3;
}

Settings* Settings::instance()
{
    static Settings s;
    return &s;
}

void Settings::load()
{
    m_file = AppPaths::settingsFile();
    m_data = QJsonObject();

    QFile f(m_file);
    if (f.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        if (doc.isObject())
            m_data = doc.object();
    }
}

void Settings::save()
{
    if (m_file.isEmpty())
        m_file = AppPaths::settingsFile();

    QSaveFile file(m_file);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(m_data).toJson(QJsonDocument::Indented));
        file.commit();
    }
}

void Settings::setValue(const QString& key, const QVariant& value)
{
    m_data.insert(key, QJsonValue::fromVariant(value));
}

QVariant Settings::value(const QString& key, const QVariant& def) const
{
    const QJsonValue v = m_data.value(key);
    if (v.isUndefined() || v.isNull())
        return def;
    return v.toVariant();
}

bool Settings::startMaximized() const              { return value(QStringLiteral("startMaximized"), false).toBool(); }
void Settings::setStartMaximized(bool v)           { setValue(QStringLiteral("startMaximized"), v); }
bool Settings::rememberWindowSize() const          { return value(QStringLiteral("rememberWindowSize"), true).toBool(); }
void Settings::setRememberWindowSize(bool v)       { setValue(QStringLiteral("rememberWindowSize"), v); }
bool Settings::rememberLastChannel() const         { return value(QStringLiteral("rememberLastChannel"), true).toBool(); }
void Settings::setRememberLastChannel(bool v)      { setValue(QStringLiteral("rememberLastChannel"), v); }
bool Settings::launchOnStartup() const             { return value(QStringLiteral("launchOnStartup"), false).toBool(); }
void Settings::setLaunchOnStartup(bool v)          { setValue(QStringLiteral("launchOnStartup"), v); }
QString Settings::lastChannelKey() const           { return value(QStringLiteral("lastChannelKey")).toString(); }
void Settings::setLastChannelKey(const QString& v) { setValue(QStringLiteral("lastChannelKey"), v); }
QByteArray Settings::windowGeometry() const
{
    // Stored as base64 text; decode back to raw bytes for restoreGeometry().
    const QString b64 = value(QStringLiteral("windowGeometry")).toString();
    return b64.isEmpty() ? QByteArray() : QByteArray::fromBase64(b64.toLatin1());
}
void Settings::setWindowGeometry(const QByteArray& v) { setValue(QStringLiteral("windowGeometry"), QString::fromLatin1(v.toBase64())); }
bool Settings::windowMaximized() const             { return value(QStringLiteral("windowMaximized"), false).toBool(); }
void Settings::setWindowMaximized(bool v)          { setValue(QStringLiteral("windowMaximized"), v); }

// Default to "disabled" (software decode): hardware decoding engages libVLC's
// D3D11 decoder, which has been the source of UI freezes on some systems.
// Users who want it can opt in through Settings -> Playback.
int Settings::hardwareAcceleration() const         { return value(QStringLiteral("hwAcceleration"), 2).toInt(); }
void Settings::setHardwareAcceleration(int v)      { setValue(QStringLiteral("hwAcceleration"), qBound(0, v, 2)); }
int Settings::bufferSizeMs() const                 { return value(QStringLiteral("bufferSizeMs"), kDefaultBufferMs).toInt(); }
void Settings::setBufferSizeMs(int v)              { setValue(QStringLiteral("bufferSizeMs"), qBound(0, v, 10000)); }
int Settings::networkTimeoutSec() const            { return value(QStringLiteral("networkTimeoutSec"), kDefaultTimeoutSec).toInt(); }
void Settings::setNetworkTimeoutSec(int v)         { setValue(QStringLiteral("networkTimeoutSec"), qBound(3, v, 120)); }
bool Settings::autoReconnect() const               { return value(QStringLiteral("autoReconnect"), true).toBool(); }
void Settings::setAutoReconnect(bool v)            { setValue(QStringLiteral("autoReconnect"), v); }
int Settings::maxRetries() const                   { return value(QStringLiteral("maxRetries"), kDefaultRetries).toInt(); }
void Settings::setMaxRetries(int v)                { setValue(QStringLiteral("maxRetries"), qBound(0, v, 10)); }
int Settings::defaultVolume() const                { return value(QStringLiteral("defaultVolume"), kDefaultVolume).toInt(); }
void Settings::setDefaultVolume(int v)             { setValue(QStringLiteral("defaultVolume"), qBound(0, v, 100)); }

QString Settings::theme() const                    { return value(QStringLiteral("theme"), QString::fromLatin1(kDefaultTheme)).toString(); }
void Settings::setTheme(const QString& v)          { setValue(QStringLiteral("theme"), v.isEmpty() ? QString::fromLatin1(kDefaultTheme) : v); }
bool Settings::compactMode() const                 { return value(QStringLiteral("compactMode"), false).toBool(); }
void Settings::setCompactMode(bool v)              { setValue(QStringLiteral("compactMode"), v); }
bool Settings::showLogos() const                   { return value(QStringLiteral("showLogos"), true).toBool(); }
void Settings::setShowLogos(bool v)                { setValue(QStringLiteral("showLogos"), v); }
QString Settings::viewMode() const                 { return value(QStringLiteral("viewMode"), QStringLiteral("grid")).toString(); }
void Settings::setViewMode(const QString& v)       { setValue(QStringLiteral("viewMode"), v); }
bool Settings::animationsEnabled() const           { return value(QStringLiteral("animationsEnabled"), true).toBool(); }
void Settings::setAnimationsEnabled(bool v)        { setValue(QStringLiteral("animationsEnabled"), v); }

int Settings::refreshIntervalMin() const           { return value(QStringLiteral("refreshIntervalMin"), 0).toInt(); }
void Settings::setRefreshIntervalMin(int v)        { setValue(QStringLiteral("refreshIntervalMin"), qBound(0, v, 1440)); }
bool Settings::autoRefreshEnabled() const          { return value(QStringLiteral("autoRefreshEnabled"), false).toBool(); }
void Settings::setAutoRefreshEnabled(bool v)       { setValue(QStringLiteral("autoRefreshEnabled"), v); }

bool Settings::checkUpdatesAutomatically() const   { return value(QStringLiteral("checkUpdatesAutomatically"), true).toBool(); }
void Settings::setCheckUpdatesAutomatically(bool v) { setValue(QStringLiteral("checkUpdatesAutomatically"), v); }

bool Settings::firstRunDone() const                { return value(QStringLiteral("firstRunDone"), false).toBool(); }
void Settings::setFirstRunDone(bool v)             { setValue(QStringLiteral("firstRunDone"), v); }
bool Settings::isOfflineOverride() const           { return value(QStringLiteral("offlineOverride"), false).toBool(); }
void Settings::setIsOfflineOverride(bool v)        { setValue(QStringLiteral("offlineOverride"), v); }

void Settings::applyLaunchOnStartup()
{
#ifdef Q_OS_WIN
    QSettings reg(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                  QSettings::NativeFormat);
    if (launchOnStartup()) {
        const QString exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        reg.setValue(QStringLiteral("STGR IpTV"), QStringLiteral("\"%1\"").arg(exe));
    } else {
        reg.remove(QStringLiteral("STGR IpTV"));
    }
#else
    // Non-Windows builds intentionally do nothing.
#endif
}
