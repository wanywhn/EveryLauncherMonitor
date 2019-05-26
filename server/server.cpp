#include "server.h"
#include <QCoreApplication>
#include <QDBusInterface>
#include <QDebug>
#include <QDir>
#include <QLoggingCategory>
#include <QStandardPaths>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/version.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/fanotify.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

QString RECOLL_CONFIG_DIR = "recoll_conf";

Q_LOGGING_CATEGORY(vfs, "vfs", QtInfoMsg)
#define vfsInfo(...) qCInfo(vfs, __VA_ARGS__)

Server::Server(QObject *parent) : QThread(parent) {
  watcher = new QFileSystemWatcher(this);
  timer = new QTimer(this);
  {
    QMutexLocker locker(&topdirsMutex);
    readConfig();
  }
  watcher->addPath(
      QString::fromStdString(theconfig->getConfDir() + "/recoll.conf"));
  connect(watcher, &QFileSystemWatcher::fileChanged, [this]() {
    this->configFileModified = true;
    {
      QMutexLocker locker(&topdirsMutex);
      readConfig();
    }
  });
  connect(timer, &QTimer::timeout, this, &Server::timeouted);
  timer->start();
}


void Server::timeouted() {
  QStringList md;
  QMutexLocker locker(&msetMutex);
  for (auto item : mset) {
    md << QString::fromStdString(item);
  }
  mset.clear();
  emit fileWrited(md);
}

void Server::readConfig() {
  std::string reason;
  auto cfgPath =
      QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
  std::string confg = cfgPath.absoluteFilePath(RECOLL_CONFIG_DIR).toStdString();
  theconfig = recollinit(0, 0, 0, reason, &confg);
  if (!theconfig || !theconfig->ok()) {
    QString msg = "Configuration problem: ";
    msg += QString::fromUtf8(reason.c_str());
    qDebug() << msg;
    exit(1);
  }
  topdirs.clear();
  for (auto item : theconfig->getTopdirs()) {
    topdirs.insert(item);
  }
  skippedPaths.clear();
  for (auto item : theconfig->getSkippedPaths()) {
    skippedPaths.insert(item);
  }
  skipeedNames.clear();
  for (auto item : theconfig->getSkippedNames()) {
    skipeedNames.insert(item);
  }
  int interval;
  theconfig->getConfParam("monitorIndexInterval",&interval);
  timer->setInterval((interval>10?interval:10)*1000);
}

void Server::run() {

  int maxfdp{0};
  fd_set rset;

  char buf[4096];
  char fdpath[32];
  char path[PATH_MAX + 1];
  ssize_t buflen, linklen;
  struct fanotify_event_metadata *metadata;
  struct timeval til;
  til.tv_sec = selectWaitTime;
  til.tv_usec = 0;

  int *fdarr{nullptr};
  for (;;) {
    int dirSize;

    {
      QMutexLocker locker(&topdirsMutex);
      dirSize = topdirs.size();
    }
    if (dirSize <= 0) {
      sleep(10);
      continue;
    }
    if (fdarr != nullptr) {
      delete[] fdarr;
      fdarr = nullptr;
    }
    fdarr = new int[dirSize];

    //创建fanotify
    FD_ZERO(&rset);
    int i = 0;
    {
      QMutexLocker locker(&topdirsMutex);
      for (auto iter = topdirs.cbegin(); iter != topdirs.cend(); ++iter, ++i) {
        //    for (unsigned int i = 0; i != dirSize; ++i) {
        CHK(fdarr[i] = fanotify_init(FAN_CLASS_NOTIF, O_RDONLY), -1);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0))

        CHK(fanotify_mark(fdarr[i], FAN_MARK_ADD | FAN_MARK_FILESYSTEM,
                          FAN_MODIFY | FAN_EVENT_ON_CHILD, AT_FDCWD,
                          iter->c_str()),
            -1);
#else
        CHK(fanotify_mark(fdarr[i], FAN_MARK_ADD | FAN_MARK_MOUNT,
                          FAN_MODIFY | FAN_EVENT_ON_CHILD, AT_FDCWD,
                          iter->c_str()),
            -1);
#endif
        FD_SET(fdarr[i], &rset);
        if (fdarr[i] > maxfdp) {
          maxfdp = fdarr[i];
        }
      }
    }
    ++maxfdp;

    //开始监控
    for (;;) {
      if (configFileModified) {
        configFileModified = false;
        break;
      }
      //      QCoreApplication::processEvents();
      auto ret = select(maxfdp, &rset, NULL, NULL, &til);
      if (ret == -1) {
        qDebug() << "select error";
      }
      til.tv_sec = selectWaitTime;
      til.tv_usec = 0;
      //出现变更
      QMutexLocker locker(&msetMutex);
      for (auto i = 0; i != dirSize; ++i) {
        if (FD_ISSET(fdarr[i], &rset)) {
          CHK(buflen = read(fdarr[i], buf, sizeof(buf)), -1);
          metadata = (struct fanotify_event_metadata *)&buf;
          while (FAN_EVENT_OK(metadata, buflen)) {
            if (metadata->mask & FAN_Q_OVERFLOW) {
              printf("Queue overflow!\n");
              continue;
            }
            sprintf(fdpath, "/proc/self/fd/%d", metadata->fd);
            CHK(linklen = readlink(fdpath, path, sizeof(path) - 1), -1);
            path[linklen] = '\0';
            auto p = std::string(path);
            mset.insert(p);
            //                        printf("%s opened by process %d.\n", path,
            //                        (int)metadata->pid);
            close(metadata->fd);
            metadata = FAN_EVENT_NEXT(metadata, buflen);
          }
          //!! 注意，要重新设置
        }
        FD_SET(fdarr[i], &rset);
      }
    }
  }
}
