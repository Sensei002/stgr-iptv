#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;

// ---------------------------------------------------------------------------
// PlaylistDialog - add or edit a playlist (name, URL/path, optional EPG URL)
// with one-click quick-add of the bundled IPTV-org community playlists.
// ---------------------------------------------------------------------------
class PlaylistDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Mode { Add, Edit };

    explicit PlaylistDialog(QWidget* parent = nullptr);

    void setMode(Mode mode);
    void setName(const QString& name);
    void setUrl(const QString& url);
    void setEpgUrl(const QString& url);

    QString name() const;
    QString url() const;
    QString epgUrl() const;
    bool refreshAfterAdd() const;
    bool isBuiltInChoice() const;

private:
    QLineEdit* m_name = nullptr;
    QLineEdit* m_url = nullptr;
    QLineEdit* m_epgUrl = nullptr;
    QComboBox* m_builtIn = nullptr;
    QLabel* m_builtInLabel = nullptr;
    QCheckBox* m_refreshAfter = nullptr;
    bool m_builtInUsed = false;
};
