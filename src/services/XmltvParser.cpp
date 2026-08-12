#include "services/XmltvParser.h"

#include <QRegularExpression>
#include <QXmlStreamReader>

namespace {

// XMLTV date format: "20260812123000 +0000"
QDateTime parseXmltvTime(const QString& raw)
{
    const QString cleaned = raw.trimmed();
    if (cleaned.size() < 14)
        return {};

    const QDateTime dt = QDateTime::fromString(cleaned.left(14), QStringLiteral("yyyyMMddHHmmss"));
    if (!dt.isValid())
        return {};
    return dt.toUTC();
}

} // namespace

QVector<XmltvParser::Program> XmltvParser::parse(const QByteArray& xml)
{
    QVector<Program> programs;
    QXmlStreamReader reader(xml);
    reader.setNamespaceProcessing(false);

    Program current;
    bool inProgramme = false;
    QString currentElement;

    while (!reader.atEnd()) {
        reader.readNext();

        if (reader.isStartElement()) {
            currentElement = reader.name().toString();

            if (currentElement == QLatin1String("programme")) {
                inProgramme = true;
                current = Program();
                current.channelId = reader.attributes().value(QStringLiteral("channel")).toString();
                current.startUtc = parseXmltvTime(reader.attributes().value(QStringLiteral("start")).toString());
                current.endUtc = parseXmltvTime(reader.attributes().value(QStringLiteral("stop")).toString());
            }
        } else if (reader.isCharacters()) {
            if (inProgramme) {
                const QString text = reader.text().toString().trimmed();
                if (currentElement == QLatin1String("title") && current.title.isEmpty())
                    current.title = text;
                else if (currentElement == QLatin1String("desc") && current.description.isEmpty())
                    current.description = text;
            }
        } else if (reader.isEndElement()) {
            const QString name = reader.name().toString();
            if (name == QLatin1String("programme")) {
                if (current.isValid())
                    programs.append(current);
                inProgramme = false;
            }
        } else if (reader.hasError()) {
            break; // tolerate truncated/malformed XML up to this point
        }
    }

    return programs;
}
