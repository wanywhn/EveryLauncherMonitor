#include <QCoreApplication>
#include <QThread>
#include <QLoggingCategory>
#include <QDBusInterface>

#include <QtDBus/QDBusConnection>

#include "server.h"
#include "everylaunchermonitor_adaptor.h"

#define DBUS_SERVER "com.gitee.wanywhn.EveryLauncherMonitor"
#define DBUS_PATH "/com/gitee/wanywhn/EveryLauncherMonitor"
#define DBUS_INTERFACE "com.gitee.wanywhn.EveryLauncherMonitor"

QString AppName = "EveryLauncher";
int main(int argc, char *argv[])
{
    qSetMessagePattern("[%{time yyyy-MM-dd, HH:mm:ss.zzz}] [%{category}-%{type}] [%{function}: %{line}]: %{message}");

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(AppName);
    QCoreApplication::setOrganizationName(AppName);

#ifdef QT_NO_DEBUG
    QLoggingCategory::setFilterRules("vfs.info=false");
#endif

    Server *server = new Server();
    auto *serverAdapter=new EveryLauncherMonitorAdaptor(server);
    QThread *workerThread=new QThread();

    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.isConnected()) {
              return -1;
          }
    if(!connection.registerService(DBUS_SERVER))
    {
        qDebug() << "error:" << connection.lastError().message();
        exit(-1);
    }
    connection.registerObject(DBUS_PATH,DBUS_INTERFACE,server);
    QObject::connect(workerThread,&QThread::started,server,&Server::myrun);
    server->moveToThread(workerThread);
    workerThread->start();
    QObject::connect(server,&Server::fileWrited,[](QStringList l){
        qDebug()<<l;
    });
    return app.exec();
}
