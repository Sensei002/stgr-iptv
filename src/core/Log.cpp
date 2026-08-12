#include "core/Log.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>
#include <QtGlobal>

#include <cstdio>

#include "core/version.h"

namespace {

QMutex s_mutex;
QFile s_file;
QTextStream s_stream(&s_file);
bool s_verbose = false;
bool s_installed = false;

#ifdef QT_NO_DEBUG
constexpr bool kBuildHasDebug = false;
#else
constexpr bool kBuildHasDebug = true;
#endif

QString levelName(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:    return QStringLiteral("DEBUG");
    case QtInfoMsg:     return QStringLiteral("INFO");
    case QtWarningMsg:  return QStringLiteral("WARN");
    case QtCriticalMsg: return QStringLiteral("ERROR");
    case QtFatalMsg:    return QStringLiteral("FATAL");
    }
    return QStringLiteral("INFO");
}

void rotateIfNeeded()
{
    if (s_file.size() <= Log::maxSizeBytes())
        return;

    s_stream.flush();
    s_file.close();

    const QString path = s_file.fileName();
    const QString oldPath = path + QStringLiteral(".1");
    QFile::remove(oldPath);
    QFile::rename(path, oldPath);

    s_file.setFileName(path);
    s_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    s_stream.setDevice(&s_file);
}

void messageHandler(QtMsgType type, const QMessageLogContext&, const QString& message)
{
    QMutexLocker lock(&s_mutex);

    if (type == QtDebugMsg && !s_verbose) {
        // Debug output is intentionally dropped unless verbose mode is on.
        return;
    }

    if (s_file.isOpen()) {
        rotateIfNeeded();

        const QString line = QStringLiteral("%1 [%2] %3\n")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")),
                 levelName(type),
                 message);
        s_stream << line;
        s_stream.flush();
    }

    if (type == QtFatalMsg) {
        // Let Qt abort with the message visible.
        fputs(qPrintable(message), stderr);
        fputc('\n', stderr);
    }
}

} // namespace

namespace Log {

void init(const QString& filePath, bool verbose)
{
    QMutexLocker lock(&s_mutex);
    s_verbose = verbose;
    s_file.setFileName(filePath);
    const bool opened = s_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    if (opened) {
        s_stream.setDevice(&s_file);
        s_stream << QStringLiteral("---- %1 started (v%2, %3 build) ----\n")
                        .arg(QStringLiteral(STGR_APP_NAME),
                             QStringLiteral(STGR_VERSION_STRING),
                             kBuildHasDebug ? QStringLiteral("debug") : QStringLiteral("release"));
        s_stream.flush();
    }
    if (!s_installed) {
        qInstallMessageHandler(messageHandler);
        s_installed = true;
    }
}

void shutdown()
{
    QMutexLocker lock(&s_mutex);
    if (s_file.isOpen()) {
        s_stream.flush();
        s_file.close();
    }
}

qint64 maxSizeBytes()
{
    return 2 * 1024 * 1024; // 2 MiB before rotating to .1
}

QString redactUrl(const QString& url)
{
    // Strip "user:password@" credentials so they never reach the log file.
    QString out = url;
    const int at = out.lastIndexOf(QLatin1Char('@'));
    if (at > 0) {
        const int scheme = out.indexOf(QLatin1String("://"));
        if (scheme >= 0 && scheme < at)
            out = out.left(scheme + 3) + out.mid(at + 1);
    }
    return out;
}

} // namespace Log
