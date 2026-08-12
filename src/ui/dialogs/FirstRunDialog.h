#pragma once

#include <QDialog>

class QPushButton;

// ---------------------------------------------------------------------------
// FirstRunDialog - the one-time welcome screen shown on first launch.
// ---------------------------------------------------------------------------
class FirstRunDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Choice { IptvOrg, AddPlaylist, ImportFile, Continue };

    explicit FirstRunDialog(QWidget* parent = nullptr);
    Choice choice() const { return m_choice; }

private:
    Choice m_choice = Choice::Continue;
    QPushButton* m_iptvOrgButton = nullptr;
    QPushButton* m_addPlaylistButton = nullptr;
    QPushButton* m_importButton = nullptr;
};
