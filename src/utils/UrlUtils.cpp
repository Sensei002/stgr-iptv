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

namespace {

// Returns the explicit scheme prefix of a URL string, lower-cased, or an
// empty string when there is none.
//
// QUrl::fromUserInput() is deliberately NOT used here: it fabricates a
// scheme for arbitrary input ("garbage-no-scheme" becomes http://garbage...,
// "some text" becomes a file: URL), which would let malformed playlist lines
// straight through the protocol whitelist.
QString extractScheme(const QString& cleaned)
{
    const int colon = cleaned.indexOf(QLatin1Char(':'));
    if (colon <= 0)
        return QString();

    const QString candidate = cleaned.left(colon);
    // A ':' inside a path or text ("C:\dir", "foo bar:baz") is not a scheme.
    if (candidate.contains(QLatin1Char('/')) || candidate.contains(QLatin1Char(' ')))
        return QString();

    return candidate.toLower();
}

} // namespace

bool isSupportedStreamUrl(const QString& raw, const QUrl& base)
{
    const QString cleaned = sanitize(raw);
    if (cleaned.isEmpty())
        return false;

    // Absolute file path from a local playlist (no scheme).
    if (QFileInfo::exists(cleaned))
        return true;

    const QString scheme = extractScheme(cleaned);
    if (!scheme.isEmpty())
        return kSupportedSchemes.contains(scheme);

    // No explicit scheme: a relative reference. Only meaningful when a
    // playlist base URL can resolve it into an absolute stream URL.
    if (base.isEmpty())
        return false;
    return !cleaned.startsWith(QLatin1String("//"))
        && !cleaned.startsWith(QLatin1Char('/'));
}

bool isHttpUrl(const QString& raw)
{
    const QString scheme = extractScheme(sanitize(raw));
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

    // An explicit scheme means the URL is absolute and self-contained.
    if (!extractScheme(cleaned).isEmpty())
        return QUrl(cleaned);

    // Relative reference - resolve against the playlist base.
    if (base.isEmpty())
        return {};
    return base.resolved(QUrl(cleaned));
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
