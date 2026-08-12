#include "services/NetworkService.h"

#include <QNetworkInformation>
#include <QNetworkRequest>

#include "core/version.h"

NetworkService* NetworkService::instance()
{
    static NetworkService s;
    return &s;
}

NetworkService::NetworkService(QObject* parent)
    : QObject(parent)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0) && QT_CONFIG(networkinformation)
    // Windows backend is available from Qt 6.7; without it we simply stay
    // "online" and let fetch results drive the heuristic instead.
    m_netInfo = QNetworkInformation::loadByName(QStringLiteral("windows"));
    if (m_netInfo) {
        setOnline(m_netInfo->isBehindCaptivePortal() ? false : m_netInfo->isOnline());
        connect(m_netInfo, &QNetworkInformation::reachabilityChanged,
                this, [this](QNetworkInformation::Reachability r) {
                    setOnline(r != QNetworkInformation::Reachability::Offline);
                });
    }
#else
    Q_UNUSED(m_netInfo);
#endif
}

void NetworkService::reportNetworkSuccess()
{
    setOnline(true);
}

void NetworkService::reportNetworkFailure()
{
    setOnline(false);
}

void NetworkService::setOnline(bool online)
{
    if (m_online == online)
        return;
    m_online = online;
    emit onlineStateChanged(m_online);
}

QNetworkRequest NetworkService::makeRequest(const QUrl& url, int timeoutMs) const
{
    QNetworkRequest request(url);
    request.setTransferTimeout(qMax(3000, timeoutMs));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("STGR-IpTV/%1 (Windows; Qt)").arg(QStringLiteral(STGR_VERSION_STRING)));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    return request;
}
