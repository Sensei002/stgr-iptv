#pragma once

#include <QDialog>
#include <QString>

// ---------------------------------------------------------------------------
// UpdateDialog - shown when the optional updater finds a newer release.
// ---------------------------------------------------------------------------
class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpdateDialog(const QString& version, const QString& releaseUrl,
                          const QString& notes, QWidget* parent = nullptr);

private:
    QString m_releaseUrl;
};
