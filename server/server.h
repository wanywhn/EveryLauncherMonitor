#ifndef SERVER_H
#define SERVER_H
#include <QDBusContext>
#include <QMutex>
#include <QThread>
#include <rclconfig.h>
#include <rclinit.h>


class Server : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface","com.gitee.wanywhn.EveryLauncherMonitor")

public:
    explicit Server(QObject *parent = nullptr);

public slots:
    void myrun();
    void setWatchPaths(QStringList paths);
    void setFileMonitorInter(int sec);
signals:
//    void resetWtachPaths(QStringList paths);
    void fileWrited(QStringList files);

private:
    QStringList watchList;
    QMutex wlMutex;
    RclConfig *theconfig;
//    std::vector<QString> monitorPaths;
    int second;
};


#endif // SERVER_H
