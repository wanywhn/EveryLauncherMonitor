#ifndef SERVER_H
#define SERVER_H
#include <QDBusContext>
#include <QMutex>
#include <QThread>


class Server : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface","com.gitee.wanywhn.everylauncherMonitor")

public:
    explicit Server(QObject *parent = nullptr);

public slots:
    void myrun();
    void setWatchPaths(QStringList paths);
signals:
//    void resetWtachPaths(QStringList paths);
    void fileWrited(QStringList files);

private:
    QStringList watchList;
    QMutex wlMutex;
};


#endif // SERVER_H
