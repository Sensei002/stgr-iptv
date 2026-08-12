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
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    // The Windows backend ships with Qt >= 6.7; without it we simply stay
    // "online" and let fetch results drive the heuristic instead. We only
    // report offline when the network is explicitly Disconnected, so an
    // unknown reachability never trips a false offline state.
    if (QNetworkInformation::loadDefaultBackend())
        m_netInfo = QNetworkInformation::instance();
    if (m_netInfo) {
        setOnline(m_netInfo->reachability() != QNetworkInformation::Reachability::Disconnected);
        connect(m_netInfo, &QNetworkInformation::reachabilityChanged,
                this, [this](QNetworkInformation::Reachability r) {
                    setOnline(r != QNetworkInformation::Reachability::Disconnected);
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
