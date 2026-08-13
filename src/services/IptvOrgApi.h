#pragma once

#include <QHash>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include "models/Channel.h"

// ---------------------------------------------------------------------------
// IptvStream - one stream entry from the iptv-org API (streams.json).
//
// The iptv-org project aggregates every known public mirror for a channel,
// each with its own URL, quality label (1080p60, 720p, ...), geo/label hints
// and - for some streams - the Referer / User-Agent header the server
// requires. That is exactly what STGR IpTV needs to:
//   * show a real quality picker per channel (not just what the M3U says),
//   * auto-failover to a working mirror when the current one stalls.
// ---------------------------------------------------------------------------
struct IptvStream {
    QString channelId;   // iptv-org channel id (e.g. "ZeeBanglaHD.in")
    QString title;       // human stream title
    QString url;         // stream URL
    QString quality;     // "1080p60", "720p", ... (may be empty)
    QString label;       // "Geo-blocked", "Requires sign-up", ... (may be empty)
    QString referrer;    // required Referer header (may be empty)
    QString userAgent;   // required User-Agent header (may be empty)
};
Q_DECLARE_METATYPE(IptvStream)

// ---------------------------------------------------------------------------
// IptvOrgApi - access to the iptv-org API (https://github.com/iptv-org/api).
//
//   * Fetches channels.json + streams.json in the background, parses them off
//     the UI thread and caches the raw JSON on disk so subsequent launches
//     (and offline use) still have mirror data.
//   * Looks up backups for a Channel by exact tvg-id first (the bundled
//     IPTV-org playlist carries tvg-id attributes that match the API channel
//     ids), falling back to a normalized-name match against channels.json.
//
// Call start() once at application startup; streamsFor() returns whatever is
// currently known (empty until the first data set is ready).
// ---------------------------------------------------------------------------
class IptvOrgApi : public QObject
{
    Q_OBJECT

public:
    static IptvOrgApi* instance();

    void start(); // idempotent; loads cache async, refreshes if stale

    // All known mirrors for a channel (best-effort match, may be empty).
    QVector<IptvStream> streamsFor(const Channel& channel) const;

    // True once cached data has been loaded (fresh or stale) or a fetch
    // completed. Mirrors are usable after this fires.
    bool isReady() const { return m_ready; }

signals:
    void ready(); // emitted when mirror data becomes (re)available

private:
    explicit IptvOrgApi(QObject* parent = nullptr);

    struct ParsedData {
        QHash<QString, QVector<IptvStream>> streamsById;
        QHash<QString, QStringList> idsByName; // normalized name -> channel ids
    };

    void loadCachedAsync();
    void refreshIfStale();
    void fetchStreams();
    void fetchChannels();
    void onFetched(const QString& which, bool ok, const QByteArray& data);
    void parseAsync(const QByteArray& streamsJson, const QByteArray& channelsJson);
    void applyData(const ParsedData& data);
    void saveCache(const QString& fileName, const QByteArray& data) const;

    QString cacheStreamsPath() const;
    QString cacheChannelsPath() const;

    QHash<QString, QVector<IptvStream>> m_streamsById;
    QHash<QString, QStringList> m_idsByName;
    bool m_started = false;
    bool m_ready = false;
    bool m_fetching = false;
    int m_pendingFetches = 0;
    QByteArray m_streamsRaw;
    QByteArray m_channelsRaw;
};
