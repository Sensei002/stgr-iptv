#pragma once

#include <QCache>
#include <QHash>
#include <QObject>
#include <QPixmap>
#include <QQueue>
#include <QSet>

class QNetworkReply;

// ---------------------------------------------------------------------------
// LogoCache - lazy, throttled, cached channel logo loading.
//
//  * Only logos actually requested by visible views are downloaded.
//  * At most kMaxConcurrentDownloads run at once; the rest wait in a queue.
//  * Successful images are stored in memory (LRU) and on disk
//    (<appdata>/logos/<md5>.img) so restarts are instant and offline use works.
//  * Broken logos are remembered for the session and never retried.
//
// All of this runs on the UI thread but never blocks it: network requests are
// asynchronous and disk hits are decoded quickly.
// ---------------------------------------------------------------------------
class LogoCache : public QObject
{
    Q_OBJECT

public:
    static LogoCache* instance();

    // Asks for a logo. Duplicate requests are coalesced.
    void request(const QString& urlKey, const QString& url);

    // Synchronous memory/disk hit check (fast).
    QPixmap cachedPixmap(const QString& urlKey) const;

    void clearMemoryCache();

signals:
    void logoReady(const QString& urlKey);

private:
    explicit LogoCache(QObject* parent = nullptr);

    struct Pending {
        QString key;
        QString url;
    };

    void startNext();
    void onReplyFinished(QNetworkReply* reply);

    // mutable: cachedPixmap() const is allowed to warm the LRU cache.
    mutable QCache<QString, QPixmap> m_memCache;
    QQueue<Pending> m_queue;
    QSet<QString> m_inFlight;
    QSet<QString> m_broken;
    QHash<QNetworkReply*, QString> m_replyKeys;

    static constexpr int kMaxConcurrentDownloads = 6;
    static constexpr int kMaxMemoryEntries = 400;
};
