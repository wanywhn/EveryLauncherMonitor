#include "server.h"
#include "vfs_change_consts.h"
#include "vfs_change_uapi.h"

#include <QCoreApplication>
#include <QDebug>
#include <QLoggingCategory>

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define OnError(message)                                                       \
    qCritical() << message << QString::fromLocal8Bit(strerror(errno));           \
    qApp->exit(errno);                                                           \
    return

Q_LOGGING_CATEGORY(vfs, "vfs", QtInfoMsg)
#define vfsInfo(...) qCInfo(vfs, __VA_ARGS__)

Server::Server(QObject *parent) : QThread(parent) {
    qRegisterMetaType<QList<QPair<QByteArray, QByteArray>>>();
}

void Server::setWatchPaths(QStringList paths) {
    // TODO sort for kernel module easy to use?
    std::sort(paths.begin(), paths.end());
    vfsInfo("setwatch want get");
    QMutexLocker locer(&wlMutex);
    vfsInfo("setwatch got");
    watchList.clear();
    watchList.append(paths);
    // int fd=open("/proc/" PROCFS_NAME,O_RDWR);
    // if(fd<0){
    //     OnError("can't open /proc");
    // }
    // ioctl_item_args item;
    // if(ioctl(fd,VC_IOCTL_DELETEITEM,&item)!=0){
    //     OnError("clear kernel watch paths failed");
    // }
    // for(auto path:paths){
    //     item.
    //           data=path.toLatin1().data();
    //     item.size=path.length()+1;
    //     if(ioctl(fd,VC_IOCTL_ADDITEM,&item)!=0){
    //         OnError("Add "+path+" Failed");
    //     }
    // }
    // close(fd);
}

void Server::run() {
    int fd = open("/proc/" PROCFS_NAME, O_RDONLY);

    if (fd < 0) {
        OnError("can't open /proc");
    }

    ioctl_wd_args wd;

    wd.condition_timeout = 10000;

    vfsInfo("before ioctl");
    while (ioctl(fd, VC_IOCTL_WAITDATA, &wd) == 0) {

        //      vfsInfo("in wile");
        if (!watchList.isEmpty()) {
            vfsInfo("while want get");
            QMutexLocker locker(&wlMutex);
            vfsInfo("while got ");
            if (!watchList.isEmpty()) {
                ioctl_item_args item;
                if (ioctl(fd, VC_IOCTL_DELETEITEM, &item) != 0) {
                    OnError("clear kernel watch paths failed");
                }
                for (auto path : watchList) {
                    item.data = path.toLatin1().data();
                    item.size = path.length() + 1;
                    if (ioctl(fd, VC_IOCTL_ADDITEM, &item) != 0) {
                        OnError("Add " + path + " Failed");
                    }
                }
                watchList.clear();
                vfsInfo("added");
            }
        }

        ioctl_rs_args irsa;

        if (ioctl(fd, VC_IOCTL_READSTAT, &irsa) != 0) {
            close(fd);
        }

        if (irsa.cur_changes == 0) {
            continue;
        }

        char buf[1 << 20];

        ioctl_rd_args ira;

        ira.data = buf;
        ira.size = sizeof(buf);
        QSet<QString> mset;

        if (ioctl(fd, VC_IOCTL_READDATA, &ira) != 0) {
        }

        // no more changes
        if (ira.size == 0) {
            continue;
        }

        int off = 0;
        for (int i = 0; i < ira.size; i++) {
            char *src = ira.data + off;
            off += strlen(src) + 1;
            printf("read: %s\n", src);
            mset.insert(QString(src));
        }
        emit fileWrited(QStringList::fromSet(mset));
    }
    close(fd);
}
