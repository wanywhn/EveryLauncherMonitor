#ifndef SERVER_H
#define SERVER_H
#include <QDBusContext>
#include <QFileSystemWatcher>
#include <QMutex>
#include <QSet>
#include <QThread>
#include <QTimer>
#include <rclconfig.h>
#include <rclinit.h>

extern QString AppName;
#define CHK(expr, errcode) if((expr)==errcode) perror(#expr), exit(EXIT_FAILURE)
class Server : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface","com.gitee.wanywhn.EveryLauncherMonitor")

public:
    explicit Server(QObject *parent = nullptr);

public slots:
    void myrun();
    void setFileMonitorInter(int sec);
signals:
    void fileWrited(QStringList files);

private slots:
    void timeouted();
private:
    void readConfig();
private:
    RclConfig *theconfig;
    std::set<std::string> topdirs;
    std::set<std::string> skippedPaths;
    std::set<std::string> skipeedNames;

    std::set<std::string> mset;

    QFileSystemWatcher *watcher;
    QTimer *timer;

    bool configFileModified{false};

};


#endif // SERVER_H
