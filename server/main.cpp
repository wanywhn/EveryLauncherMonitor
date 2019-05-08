#include <QCoreApplication>
#include <QDBusInterface>
#include <QLoggingCategory>
#include <QThread>

#include <QtDBus/QDBusConnection>

#include "everylaunchermonitor_adaptor.h"
#include "server.h"

#define DBUS_SERVER "com.gitee.wanywhn.EveryLauncherMonitor"
#define DBUS_PATH "/com/gitee/wanywhn/EveryLauncherMonitor"
#define DBUS_INTERFACE "com.gitee.wanywhn.EveryLauncherMonitor"

QString AppName = "EveryLauncher";
int main(int argc, char *argv[]) {
  qSetMessagePattern("[%{time yyyy-MM-dd, HH:mm:ss.zzz}] [%{category}-%{type}] "
                     "[%{function}: %{line}]: %{message}");

  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationName(AppName);
  QCoreApplication::setOrganizationName(AppName);

  Server *server = new Server();
  auto *serverAdapter = new EveryLauncherMonitorAdaptor(server);

  QDBusConnection connection = QDBusConnection::sessionBus();
  if (!connection.isConnected()) {
    return -1;
  }
  if (!connection.registerService(DBUS_SERVER)) {
    qDebug() << "error:" << connection.lastError().message();
    exit(-1);
  }
  connection.registerObject(DBUS_PATH, DBUS_INTERFACE, server);
  server->start();
  //    workerThread->start();
  QObject::connect(server, &Server::fileWrited,
                   [](QStringList l) { qDebug() << l; });
  return app.exec();
}
