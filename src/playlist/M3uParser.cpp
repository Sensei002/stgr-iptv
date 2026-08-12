#include "playlist/M3uParser.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>

#include "utils/StringUtils.h"
#include "utils/UrlUtils.h"

namespace {

// key="value", key='value' or key=value tokens.
const QRegularExpression kAttrRe(
    QStringLiteral("([A-Za-z][A-Za-z0-9_-]*)\\s*=\\s*(?:\"([^\"]*)\"|'([^']*)'|([^\\s]+))"));

QStringList splitLines(const QString& text)
{
    QStringList lines;
    QString current;
    current.reserve(256);

    const auto flush = [&lines, &current]() {
        lines.append(current);
        current.clear();
    };

    for (const QChar& c : text) {
        if (c == QLatin1Char('\n') || c == QLatin1Char('\r')) {
            flush();
        } else {
            current.append(c);
        }
    }
    if (!current.isEmpty())
        flush();
    return lines;
}

// Parses the attribute section of an #EXTINF line into a map.
QHash<QString, QString> parseAttributes(const QString& attrs)
{
    QHash<QString, QString> map;
    auto it = kAttrRe.globalMatch(attrs);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        const QString key = m.captured(1).toLower();
        QString value = m.captured(2);
        if (value.isEmpty())
            value = m.captured(3);
        if (value.isEmpty())
            value = m.captured(4);
        map.insert(key, StringUtils::stripQuotes(value));
    }
    return map;
}

} // namespace

M3uParser::Result M3uParser::parse(const QByteArray& data,
                                   const QUrl& baseUrl,
                                   const QString& playlistId,
                                   const QString& playlistName,
                                   int maxChannels)
{
    Result result;
    if (maxChannels <= 0)
        return result;

    QByteArray raw = data;
    if (raw.startsWith("\xEF\xBB\xBF"))
        raw = raw.mid(3); // strip UTF-8 BOM

    const QString text = QString::fromUtf8(raw);
    const QStringList lines = splitLines(text);

    QHash<QString, QString> pendingAttrs;
    QString pendingGroup;
    QSet<QString> seenUrls;

    const auto flushPending = [&pendingAttrs, &pendingGroup]() {
        pendingAttrs.clear();
        pendingGroup.clear();
    };

    for (const QString& rawLine : lines) {
        const QString line = rawLine.trimmed();

        if (line.isEmpty())
            continue;

        if (line.startsWith(QLatin1String("#EXTM3U"))) {
            continue; // header line
        }

        if (line.startsWith(QLatin1String("#EXTINF:"))) {
            flushPending();

            // "#EXTINF:<duration> <attrs>,<title>" - split at the first comma
            // that sits *outside* quoted attribute values.
            const QString body = line.mid(8);
            int comma = -1;
            QChar quote(0);
            for (int i = 0; i < body.size(); ++i) {
                const QChar c = body.at(i);
                if (c == QLatin1Char('"') || c == QLatin1Char('\'')) {
                    if (quote.isNull())
                        quote = c;
                    else if (quote == c)
                        quote = QChar(0);
                } else if (c == QLatin1Char(',') && quote.isNull()) {
                    comma = i;
                    break;
                }
            }
            QString attrs;
            QString title;
            if (comma >= 0) {
                attrs = body.left(comma);
                title = body.mid(comma + 1);
            } else {
                attrs = body;
            }
            pendingAttrs = parseAttributes(attrs);
            if (pendingAttrs.contains(QStringLiteral("tvg-name")) && title.isEmpty())
                title = pendingAttrs.value(QStringLiteral("tvg-name"));
            if (!title.isEmpty())
                pendingAttrs.insert(QStringLiteral("_title"),
                                    StringUtils::stripQuotes(StringUtils::clean(title)));
            continue;
        }

        if (line.startsWith(QLatin1String("#EXTGRP:"))) {
            pendingGroup = StringUtils::clean(line.mid(8));
            continue;
        }

        if (line.startsWith(QLatin1Char('#'))) {
            // Any other tag (comments, #EXT-X-... HLS markers, ...).
            continue;
        }

        // --- stream URL line ---
        const QString cleaned = UrlUtils::sanitize(line);
        if (cleaned.isEmpty()) {
            ++result.warnings;
            continue;
        }

        if (!UrlUtils::isSupportedStreamUrl(cleaned)) {
            ++result.warnings;
            flushPending();
            continue;
        }

        const QUrl resolved = UrlUtils::resolveAgainst(cleaned, baseUrl);
        const bool hasResolvableTarget = !resolved.scheme().isEmpty()
            || QFileInfo::exists(resolved.toLocalFile())
            || QFileInfo::exists(cleaned);
        if (!hasResolvableTarget) {
            ++result.warnings;
            flushPending();
            continue;
        }

        const QString resolvedUrl = resolved.toString();
        if (seenUrls.contains(resolvedUrl)) {
            ++result.warnings; // duplicate channel
            flushPending();
            continue;
        }
        seenUrls.insert(resolvedUrl);

        Channel ch;
        ch.name = pendingAttrs.value(QStringLiteral("_title"));
        if (ch.name.isEmpty())
            ch.name = pendingAttrs.value(QStringLiteral("tvg-name"));
        ch.id = pendingAttrs.value(QStringLiteral("tvg-id"));
        ch.logo = pendingAttrs.value(QStringLiteral("tvg-logo"));
        ch.group = pendingAttrs.value(QStringLiteral("group-title"));
        ch.country = pendingAttrs.value(QStringLiteral("tvg-country"));
        ch.language = pendingAttrs.value(QStringLiteral("tvg-language"));
        ch.network = pendingAttrs.value(QStringLiteral("tvg-network"));
        ch.number = pendingAttrs.value(QStringLiteral("tvg-chno")).toInt();
        ch.url = resolvedUrl;
        ch.playlistId = playlistId;
        ch.playlistName = playlistName;

        if (ch.group.isEmpty())
            ch.group = pendingGroup;
        if (ch.name.isEmpty())
            ch.name = resolvedUrl;
        if (!ch.group.isEmpty())
            ch.group = StringUtils::clean(ch.group);

        result.channels.append(ch);
        flushPending();

        if (result.channels.size() >= maxChannels) {
            result.truncated = result.channels.size();
            break;
        }
    }

    // Assign stable position numbers to channels without tvg-chno.
    for (int i = 0; i < result.channels.size(); ++i) {
        if (result.channels[i].number <= 0)
            result.channels[i].number = i + 1;
    }

    return result;
}
