#pragma once

#include <QObject>
#include <QString>

// ---------------------------------------------------------------------------
// UpdateService - optional update check against GitHub Releases.
//
// * Only ever talks to the official repository (STGR_UPDATER_REPO, defined
//   at build time) over HTTPS.
// * Never forces updates; the UI decides what to show.
// * Pre-release versions are ignored (the /releases/latest endpoint).
// * No installer is downloaded automatically - "Update" just opens the
//   release page in the browser.
// ---------------------------------------------------------------------------
class UpdateService : public QObject
{
    Q_OBJECT

public:
    static UpdateService* instance();

    void checkForUpdates(); // asynchronous

    QString repo() const { return m_repo; }

signals:
    void updateAvailable(const QString& version, const QString& releaseUrl,
                         const QString& notes);
    void noUpdateAvailable();
    void updateCheckFailed(const QString& errorMessage);

private:
    explicit UpdateService(QObject* parent = nullptr);

    QString m_repo;
};
