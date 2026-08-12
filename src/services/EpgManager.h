#pragma once

#include <QHash>
#include <QObject>
#include <QVector>

#include "models/Playlist.h"
#include "services/XmltvParser.h"

// ---------------------------------------------------------------------------
// EpgManager - loads XMLTV data per playlist (modular, optional) and answers
// "what's on now / next" for a channel.
//
// EPG is deliberately optional: nothing depends on it, and a missing or
// failing EPG source never affects playback. Programs are indexed by tvg-id.
// ---------------------------------------------------------------------------
class EpgManager : public QObject
{
    Q_OBJECT

public:
    static EpgManager* instance();

    // Loads (or reloads) EPG data for a playlist from its epgUrl.
    void loadForPlaylist(const Playlist& playlist);

    bool hasData() const { return !m_programs.isEmpty(); }
    XmltvParser::Program currentProgram(const QString& tvgId) const;
    XmltvParser::Program nextProgram(const QString& tvgId) const;

signals:
    void epgLoaded(const QString& playlistId);
    void epgFailed(const QString& playlistId, const QString& errorMessage);

private:
    explicit EpgManager(QObject* parent = nullptr);
    void onParsed(const QString& playlistId, const QVector<XmltvParser::Program>& programs);

    QHash<QString, QVector<XmltvParser::Program>> m_programs; // tvgId -> programs (by start time)
};
