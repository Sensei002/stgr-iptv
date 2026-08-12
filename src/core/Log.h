#pragma once

#include <QString>

// ---------------------------------------------------------------------------
// Log - tiny local-only, privacy-friendly file logger.
//
// Writes to <appdata>/logs/stgr-iptv.log with automatic rotation (the file
// is rolled to stgr-iptv.log.1 once it grows past the size cap). Logs stay
// on the local machine and are never uploaded anywhere.
//
// Verbose/debug output is compiled out of Release builds. In addition, call
// sites should pass URLs through Log::redactUrl() so credentials embedded in
// streams never reach the log file.
// ---------------------------------------------------------------------------
namespace Log {

// Installs the message handler. verbose enables Debug-level output.
void init(const QString& filePath, bool verbose);
void shutdown();

// Cap after which the log file rotates (bytes).
qint64 maxSizeBytes();

// Removes userinfo (user:pass@) from a URL string for safe logging.
QString redactUrl(const QString& url);

} // namespace Log
