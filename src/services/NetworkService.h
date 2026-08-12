#pragma once

#include <QNetworkAccessManager>
#include <QObject>

class QNetworkInformation;

// ---------------------------------------------------------------------------
// NetworkService - single shared QNetworkAccessManager plus best-effort
// online/offline tracking.
//
// Offline tracking is deliberately conservative: a network failure reported
// by playlist/logo/update fetches flips the flag, a success flips it back.
// When Qt >= 6.7 provides QNetworkInformation, it is used as the primary
// signal instead.
// ---------------------------------------------------------------------------
class NetworkService : public QObject
{
    Q_OBJECT

public:
    static NetworkService* instance();

    QNetworkAccessManager* nam() { return &m_nam; }
    bool isOnline() const { return m_online; }

    // Called by fetchers when a request definitively succeeded / failed.
    void reportNetworkSuccess();
    void reportNetworkFailure();

    // Creates a GET request with a transfer timeout and a polite user agent.
    QNetworkRequest makeRequest(const QUrl& url, int timeoutMs) const;

signals:
    void onlineStateChanged(bool online);

private:
    explicit NetworkService(QObject* parent = nullptr);
    void setOnline(bool online);

    QNetworkAccessManager m_nam;
    QNetworkInformation* m_netInfo = nullptr;
    bool m_online = true;
};
