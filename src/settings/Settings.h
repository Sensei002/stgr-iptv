#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVariant>

// ---------------------------------------------------------------------------
// Settings - typed access to the JSON settings file
// (<appdata>/settings/settings.json). No registry, no telemetry; every
// preference is plain JSON so it survives upgrades and portable copies.
// ---------------------------------------------------------------------------
class Settings : public QObject
{
    Q_OBJECT

public:
    static Settings* instance();

    void load();
    void save();

    // --- General -----------------------------------------------------------
    bool startMaximized() const;
    void setStartMaximized(bool v);
    bool rememberWindowSize() const;
    void setRememberWindowSize(bool v);
    bool rememberLastChannel() const;
    void setRememberLastChannel(bool v);
    bool launchOnStartup() const;
    void setLaunchOnStartup(bool v);
    QString lastChannelKey() const;
    void setLastChannelKey(const QString& v);
    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray& v);
    bool windowMaximized() const;
    void setWindowMaximized(bool v);

    // --- Playback ------------------------------------------------------------
    // 0 = automatic, 1 = enabled, 2 = disabled
    int hardwareAcceleration() const;
    void setHardwareAcceleration(int v);
    int bufferSizeMs() const;          // VLC network-caching
    void setBufferSizeMs(int v);
    int networkTimeoutSec() const;     // HTTP transfer timeout
    void setNetworkTimeoutSec(int v);
    bool autoReconnect() const;
    void setAutoReconnect(bool v);
    int maxRetries() const;
    void setMaxRetries(int v);
    int defaultVolume() const;         // 0..100
    void setDefaultVolume(int v);

    // --- Interface ------------------------------------------------------------
    QString theme() const;             // v1: "dark"
    void setTheme(const QString& v);
    bool compactMode() const;
    void setCompactMode(bool v);
    bool showLogos() const;
    void setShowLogos(bool v);
    QString viewMode() const;          // "grid" | "list"
    void setViewMode(const QString& v);
    bool animationsEnabled() const;
    void setAnimationsEnabled(bool v);

    // --- Playlists -------------------------------------------------------------
    int refreshIntervalMin() const;    // 0 = never
    void setRefreshIntervalMin(int v);
    bool autoRefreshEnabled() const;
    void setAutoRefreshEnabled(bool v);

    // --- Updates ---------------------------------------------------------------
    bool checkUpdatesAutomatically() const;
    void setCheckUpdatesAutomatically(bool v);

    // --- Misc ------------------------------------------------------------------
    bool firstRunDone() const;
    void setFirstRunDone(bool v);
    bool isOfflineOverride() const;    // sticky "you're offline" indicator
    void setIsOfflineOverride(bool v);

    // Applies the "launch on startup" preference via the HKCU Run key (Windows).
    void applyLaunchOnStartup();

private:
    Settings() = default;
    void setValue(const QString& key, const QVariant& value);
    QVariant value(const QString& key, const QVariant& def = QVariant()) const;

    QJsonObject m_data;
    QString m_file;
};
