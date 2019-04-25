#include <QCoreApplication>
#include <QThread>
#include <QLoggingCategory>

#include "server.h"


int main(int argc, char *argv[])
{
    qSetMessagePattern("[%{time yyyy-MM-dd, HH:mm:ss.zzz}] [%{category}-%{type}] [%{function}: %{line}]: %{message}");

    QCoreApplication app(argc, argv);

#ifdef QT_NO_DEBUG
    QLoggingCategory::setFilterRules("vfs.info=false");
#endif

    Server *server = new Server();

    //TODO DBUS set watchpaths ,clear watchpaths,notify changes

    server->start();

    server->setWatchPaths(QStringList("/home/ubuntu"));
    QObject::connect(server,&Server::fileWrited,[](QStringList l){
        for (auto item:l){
            qDebug()<<("main get :"+item);


                     }
    });
    return app.exec();
}
