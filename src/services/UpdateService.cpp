#include "services/UpdateService.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>

#include "core/version.h"
#include "services/NetworkService.h"
#include "utils/StringUtils.h"

UpdateService* UpdateService::instance()
{
    static UpdateService s;
    return &s;
}

UpdateService::UpdateService(QObject* parent)
    : QObject(parent)
    , m_repo(QStringLiteral(STGR_UPDATER_REPO))
{
}

void UpdateService::checkForUpdates()
{
    if (m_repo.isEmpty())
        return;

    const QUrl url(QStringLiteral("https://api.github.com/repos/%1/releases/latest").arg(m_repo));
    QNetworkRequest request = NetworkService::instance()->makeRequest(url, 15000);
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");

    QNetworkReply* reply = NetworkService::instance()->nam()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::OperationCanceledError)
            return;

        // 404 (no releases yet / repo not reachable) is treated as "no update".
        if (reply->error() != QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 404) {
            emit updateCheckFailed(tr("Could not check for updates: %1").arg(reply->errorString()));
            return;
        }
        if (reply->error() == QNetworkReply::NoError) {
            NetworkService::instance()->reportNetworkSuccess();

            const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
            const QString tag = root.value(QStringLiteral("tag_name")).toString();
            const QString releaseUrl = root.value(QStringLiteral("html_url")).toString();
            QString notes = root.value(QStringLiteral("body")).toString().trimmed();
            if (notes.size() > 2000)
                notes = notes.left(2000) + QStringLiteral("\u2026");

            QString version = tag;
            if (version.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
                version = version.mid(1);

            if (version.isEmpty()) {
                emit updateCheckFailed(tr("Update check returned no version information."));
                return;
            }

            if (StringUtils::compareVersions(version, QStringLiteral(STGR_VERSION_STRING)) > 0)
                emit updateAvailable(version, releaseUrl, notes);
            else
                emit noUpdateAvailable();
        } else {
            emit noUpdateAvailable(); // 404 path
        }
    });
}
