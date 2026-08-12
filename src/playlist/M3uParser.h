#pragma once

#include <QByteArray>
#include <QString>
#include <QUrl>
#include <QVector>

#include "models/Channel.h"

// ---------------------------------------------------------------------------
// M3uParser - parses M3U/M3U8 playlist data into Channel records.
//
// Design rules:
//  * Pure and stateless: safe to call on any worker thread.
//  * Never throws, never crashes on malformed external input.
//  * Handles UTF-8 (incl. BOM), CRLF / LF / lone CR, missing metadata,
//    malformed attribute lines, duplicate URLs and huge playlists.
//  * Only whitelisted streaming protocols survive (see UrlUtils).
//  * Relative stream URLs are resolved against the playlist's own URL.
// ---------------------------------------------------------------------------
class M3uParser
{
public:
    struct Result {
        QVector<Channel> channels;
        int warnings = 0;   // lines that were skipped as unusable
        int truncated = 0;  // non-zero when the channel cap was hit
    };

    static constexpr int kDefaultMaxChannels = 100000;

    static Result parse(const QByteArray& data,
                        const QUrl& baseUrl,
                        const QString& playlistId,
                        const QString& playlistName,
                        int maxChannels = kDefaultMaxChannels);
};
