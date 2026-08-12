#include "utils/UrlUtils.h"

#include <QFileInfo>

namespace UrlUtils {

static const QStringList kSupportedSchemes = {
    QStringLiteral("http"),
    QStringLiteral("https"),
    QStringLiteral("rtsp"),
    QStringLiteral("rtsps"),
    QStringLiteral("rtmp"),
    QStringLiteral("rtmps"),
    QStringLiteral("rtp"),
    QStringLiteral("udp"),
    QStringLiteral("mms"),
    QStringLiteral("mmsh"),
    QStringLiteral("file"),
};

bool isSupportedStreamUrl(const QString& raw)
{
    const QString cleaned = sanitize(raw);
    if (cleaned.isEmpty())
        return false;

    // Absolute file path from a local playlist (no scheme).
    if (QFileInfo::exists(cleaned))
        return true;

    const QUrl url = QUrl::fromUserInput(cleaned);
    const QString scheme = url.scheme().toLower();
    if (scheme.isEmpty()) {
        // Relative reference - only meaningful when a playlist base resolves it.
        return !cleaned.startsWith(QLatin1String("//"))
            && !cleaned.startsWith(QLatin1Char('/'));
    }
    return kSupportedSchemes.contains(scheme);
}

bool isHttpUrl(const QString& raw)
{
    const QUrl url = QUrl::fromUserInput(sanitize(raw));
    const QString scheme = url.scheme().toLower();
    return scheme == QLatin1String("http") || scheme == QLatin1String("https");
}

QString sanitize(QString raw)
{
    raw = raw.trimmed();
    if (raw.size() >= 2) {
        const QChar first = raw.front();
        const QChar last = raw.back();
        if ((first == QLatin1Char('"') && last == QLatin1Char('"'))
            || (first == QLatin1Char('\'') && last == QLatin1Char('\''))) {
            raw = raw.mid(1, raw.size() - 2);
        }
    }
    // Remove any control characters (tabs, newlines, null bytes, ...).
    QString out;
    out.reserve(raw.size());
    for (const QChar& c : raw) {
        // QChar::isControl() was removed in Qt 6 - use the Unicode category.
        if (c.category() != QChar::Other_Control)
            out.append(c);
    }
    return out.trimmed();
}

QUrl resolveAgainst(const QString& raw, const QUrl& base)
{
    const QString cleaned = sanitize(raw);
    if (cleaned.isEmpty())
        return {};

    QUrl url = QUrl::fromUserInput(cleaned);
    if (url.scheme().isEmpty() && !base.isEmpty()) {
        url = base.resolved(url);
    }
    return url;
}

QString redact(const QString& url)
{
    QString out = url;
    const int at = out.lastIndexOf(QLatin1Char('@'));
    if (at > 0) {
        const int scheme = out.indexOf(QLatin1String("://"));
        if (scheme >= 0 && scheme < at)
            out = out.left(scheme + 3) + out.mid(at + 1);
    }
    return out;
}

} // namespace UrlUtils
