#pragma once

#include <QString>
#include <QStringList>

// ---------------------------------------------------------------------------
// StringUtils - small, dependency-free helpers used across the application.
// ---------------------------------------------------------------------------
namespace StringUtils {

// Trims and collapses runs of whitespace into single spaces.
QString clean(const QString& input);

// Removes surrounding single/double quotes.
QString stripQuotes(QString input);

// Splits on ',' and ':' producing trimmed, lowercased, non-empty tokens.
QStringList tokenize(const QString& input);

// True when every token is found (substring, case-insensitive) in lower.
// lower must already be lowercased for speed.
bool matchesAllTokens(const QString& lower, const QStringList& tokens);

// "sports_news" -> "Sports News"; "bbc-news" -> "BBC News".
QString humanize(const QString& input);

// Compares dotted numeric versions ("1.2.10" > "1.2.9"). Returns <0, 0, >0.
int compareVersions(const QString& a, const QString& b);

} // namespace StringUtils
