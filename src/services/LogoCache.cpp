#include "services/LogoCache.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QNetworkReply>
#include <QPixmap>

#include "core/AppPaths.h"
#include "services/NetworkService.h"

LogoCache* LogoCache::instance()
{
    static LogoCache s;
    return &s;
}

LogoCache::LogoCache(QObject* parent)
    : QObject(parent)
    , m_memCache(kMaxMemoryEntries)
{
}

QPixmap LogoCache::cachedPixmap(const QString& urlKey) const
{
    if (urlKey.isEmpty())
        return {};

    if (QPixmap* p = m_memCache.object(urlKey))
        return *p;

    // Disk hit?
    const QString file = AppPaths::logosDir() + QLatin1Char('/')
        + QString::fromLatin1(QCryptographicHash::hash(urlKey.toUtf8(), QCryptographicHash::Md5).toHex())
        + QStringLiteral(".img");
    QFile f(file);
    if (f.open(QIODevice::ReadOnly)) {
        QPixmap p;
        if (p.loadFromData(f.readAll())) {
            m_memCache.insert(urlKey, new QPixmap(p));
            return p;
        }
    }
    return {};
}

void LogoCache::request(const QString& urlKey, const QString& url)
{
    if (urlKey.isEmpty() || url.isEmpty())
        return;
    if (m_memCache.contains(urlKey) || m_broken.contains(urlKey) || m_inFlight.contains(urlKey))
        return;

    // Serve straight from disk without a network round trip.
    if (!cachedPixmap(urlKey).isNull()) {
        emit logoReady(urlKey);
        return;
    }

    m_queue.enqueue({ urlKey, url });
    startNext();
}

void LogoCache::startNext()
{
    while (m_inFlight.size() < kMaxConcurrentDownloads && !m_queue.isEmpty()) {
        const Pending pending = m_queue.dequeue();
        if (m_broken.contains(pending.key) || m_memCache.contains(pending.key))
            continue;

        m_inFlight.insert(pending.key);
        QNetworkReply* reply = NetworkService::instance()->nam()->get(
            NetworkService::instance()->makeRequest(QUrl(pending.url), 10000));
        m_replyKeys.insert(reply, pending.key);
        connect(reply, &QNetworkReply::finished, this,
                [this, reply]() { onReplyFinished(reply); });
    }
}

void LogoCache::onReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    const QString key = m_replyKeys.take(reply);
    m_inFlight.remove(key);
    if (key.isEmpty())
        return;

    const bool ok = reply->error() == QNetworkReply::NoError;
    if (ok)
        NetworkService::instance()->reportNetworkSuccess();
    else if (reply->error() == QNetworkReply::ConnectionRefusedError
             || reply->error() == QNetworkReply::HostNotFoundError
             || reply->error() == QNetworkReply::TimeoutError)
        NetworkService::instance()->reportNetworkFailure();

    if (ok && reply->bytesAvailable() > 0) {
        const QByteArray data = reply->readAll();
        QPixmap p;
        if (p.loadFromData(data)) {
            m_memCache.insert(key, new QPixmap(p));

            // Persist to disk for future sessions.
            const QString file = AppPaths::logosDir() + QLatin1Char('/')
                + QString::fromLatin1(QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex())
                + QStringLiteral(".img");
            QFile f(file);
            if (f.open(QIODevice::WriteOnly)) {
                f.write(data);
                f.close();
            }
            emit logoReady(key);
        } else {
            m_broken.insert(key);
        }
    } else {
        m_broken.insert(key);
    }

    startNext();
}

void LogoCache::clearMemoryCache()
{
    m_memCache.clear();
}
