#include <QApplication>
#include <QFont>
#include <QMetaType>

#include "core/AppPaths.h"
#include "core/Log.h"
#include "core/version.h"
#include "models/Channel.h"
#include "ui/MainWindow.h"
#include "ui/Theme.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName(QStringLiteral("STGR IpTV"));
    QApplication::setApplicationVersion(QStringLiteral(STGR_VERSION_STRING));
    QApplication::setOrganizationName(QStringLiteral("steigerdojo"));
    QApplication::setOrganizationDomain(QStringLiteral("steigerdojo.com"));

    qRegisterMetaType<Channel>("Channel");

    // Storage layout + logging must exist before anything logs.
    AppPaths::ensureDirs();
    Log::init(AppPaths::logFile(), /*verbose=*/false);

    // Visual identity: Fusion style + dark dojo palette + stylesheet.
    app.setStyle(QStringLiteral("Fusion"));
    app.setPalette(Theme::palette());
    app.setFont(QFont(QStringLiteral("Segoe UI"), 10));
    app.setStyleSheet(Theme::stylesheet());
    app.setWindowIcon(Theme::appIcon());

    MainWindow window;
    window.show();

    const int code = app.exec();

    Log::shutdown();
    return code;
}
