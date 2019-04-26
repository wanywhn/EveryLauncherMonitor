#include <QCoreApplication>
#include <QThread>
#include <QLoggingCategory>
#include <QDBusInterface>

#include <QtDBus/QDBusConnection>

#include "server.h"
#include "everylaunchermonitor_adaptor.h"

#define DBUS_SERVER "com.gitee.wanywhn.everylauncherMonitor"
#define DBUS_PATH "/com/gitee/wanywhn/everylauncherMonitor"
#define DBUS_INTERFACE "com.gitee.wanywhn.everylauncherMonitor"

int main(int argc, char *argv[])
{
    qSetMessagePattern("[%{time yyyy-MM-dd, HH:mm:ss.zzz}] [%{category}-%{type}] [%{function}: %{line}]: %{message}");

    QCoreApplication app(argc, argv);

#ifdef QT_NO_DEBUG
    QLoggingCategory::setFilterRules("vfs.info=false");
#endif

    Server *server = new Server();
    auto *serverAdapter=new EverylauncherMonitorAdaptor(server);
    QThread *workerThread=new QThread();

    //TODO DBUS set watchpaths ,clear watchpaths,notify changes

    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.isConnected()) {
              fprintf(stderr, "Cannot connect to the D-Bus session bus.\n"
                      "To start it, run:\n"
                      "\teval `dbus-launch --auto-syntax`\n");
              return 1;
          }
    if(!connection.registerService(DBUS_SERVER))
    {
        qDebug() << "error:" << connection.lastError().message();
        exit(-1);
    }
    connection.registerObject(DBUS_PATH,DBUS_INTERFACE,server);
    QObject::connect(workerThread,&QThread::started,server,&Server::myrun);

   // static QDBusInterface notifyApp("com.gitee.wanywhn.everylauncher",
   //                                 "/com/gitee/wanywhn/everylauncher",
   //                                 "com.gitee.wanywhn.everylauncher");
   // if(!notifyApp.isValid()){
   //     qDebug()<<"notify is not valid";
   //     return 0;
   // }
    server->moveToThread(workerThread);
    serverAdapter->moveToThread(workerThread);
    workerThread->start();
//    QObject::connect(server,&Server::fileWrited,[](QStringList l){
//                        notifyApp.call( "fileWrited",l);
//    });
    return app.exec();
}
