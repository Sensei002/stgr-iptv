#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QVector>

// ---------------------------------------------------------------------------
// XmltvParser - minimal, dependency-free XMLTV parser.
//
// Extracts <programme> records (channel, start, stop, title, description).
// Malformed entries are skipped; the parser never throws and can be called
// from a worker thread. Times are parsed as UTC and converted to local time
// by the caller when needed.
// ---------------------------------------------------------------------------
class XmltvParser
{
public:
    struct Program {
        QString channelId;   // matches tvg-id
        QString title;
        QString description;
        QDateTime startUtc;
        QDateTime endUtc;

        bool isValid() const { return !channelId.isEmpty() && startUtc.isValid(); }
    };

    static QVector<Program> parse(const QByteArray& xml);
};
