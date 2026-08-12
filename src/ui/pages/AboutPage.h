#pragma once

#include <QWidget>

class QPushButton;

// ---------------------------------------------------------------------------
// AboutPage - branding, version, legal disclaimer and links.
// ---------------------------------------------------------------------------
class AboutPage : public QWidget
{
    Q_OBJECT

public:
    explicit AboutPage(QWidget* parent = nullptr);

signals:
    void checkForUpdatesRequested();

private:
    QPushButton* m_updateButton = nullptr;
};
