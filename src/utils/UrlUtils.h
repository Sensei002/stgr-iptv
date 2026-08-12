#pragma once

#include <QString>
#include <QUrl>

// ---------------------------------------------------------------------------
// UrlUtils - every URL that reaches the player or the network is treated as
// untrusted input and is validated here first.
//
// Only a whitelist of streaming protocols is accepted (see kSupportedSchemes).
// This blocks accidental launching of arbitrary schemes (javascript:, data:,
// file: for remote playlists, custom handlers, ...) coming from M3U files.
// ---------------------------------------------------------------------------
namespace UrlUtils {

// True when the URL uses one of the supported streaming protocols.
// Relative references (no explicit scheme) are only accepted when a playlist
// base URL is available to resolve them into absolute stream URLs.
bool isSupportedStreamUrl(const QString& raw, const QUrl& base = QUrl());

// True when the URL is an http/https resource.
bool isHttpUrl(const QString& raw);

// Cleans untrusted input: strips quotes, whitespace and control characters.
QString sanitize(QString raw);

// Resolves a raw URL against a playlist base (for relative entries).
QUrl resolveAgainst(const QString& raw, const QUrl& base);

// Same redaction as Log::redactUrl, available without including the logger.
QString redact(const QString& url);

} // namespace UrlUtils
