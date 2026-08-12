#include "utils/StringUtils.h"

namespace StringUtils {

QString clean(const QString& input)
{
    QString out = input.trimmed();
    out = out.simplified();
    return out;
}

QString stripQuotes(QString input)
{
    input = input.trimmed();
    if (input.size() >= 2) {
        const QChar first = input.front();
        const QChar last = input.back();
        if ((first == QLatin1Char('"') && last == QLatin1Char('"'))
            || (first == QLatin1Char('\'') && last == QLatin1Char('\''))) {
            input = input.mid(1, input.size() - 2);
        }
    }
    return input.trimmed();
}

QStringList tokenize(const QString& input)
{
    const QString cleaned = clean(input).toLower();
    if (cleaned.isEmpty())
        return {};

    QStringList tokens;
    QString current;
    const auto appendToken = [&tokens, &current]() {
        if (!current.isEmpty()) {
            tokens.append(current);
            current.clear();
        }
    };

    for (const QChar& c : cleaned) {
        if (c.isLetterOrNumber()) {
            current.append(c);
        } else {
            appendToken();
        }
    }
    appendToken();
    return tokens;
}

bool matchesAllTokens(const QString& lower, const QStringList& tokens)
{
    for (const QString& token : tokens) {
        if (token.isEmpty())
            continue;
        if (!lower.contains(token))
            return false;
    }
    return true;
}

QString humanize(const QString& input)
{
    QString out = clean(input);
    if (out.isEmpty())
        return out;

    out.replace(QLatin1Char('_'), QLatin1Char(' '));
    out.replace(QLatin1Char('-'), QLatin1Char(' '));
    out = out.simplified();

    // Capitalize the first letter of each word, leaving the rest untouched
    // (so "BBC News" stays "BBC News" and "sports news" becomes "Sports news").
    QStringList words = out.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (QString& word : words) {
        if (!word.isEmpty() && word.front().isLower())
            word[0] = word.front().toUpper();
    }
    return words.join(QLatin1Char(' '));
}

int compareVersions(const QString& a, const QString& b)
{
    const QStringList pa = a.split(QLatin1Char('.'), Qt::SkipEmptyParts);
    const QStringList pb = b.split(QLatin1Char('.'), Qt::SkipEmptyParts);

    const int n = qMax(pa.size(), pb.size());
    for (int i = 0; i < n; ++i) {
        const int na = (i < pa.size()) ? pa.at(i).toInt() : 0;
        const int nb = (i < pb.size()) ? pb.at(i).toInt() : 0;
        if (na != nb)
            return na < nb ? -1 : 1;
    }
    return 0;
}

} // namespace StringUtils
