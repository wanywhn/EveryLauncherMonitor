#ifndef SERVER_H
#define SERVER_H


#include <QMutex>
#include <QThread>


class Server : public QThread
{
    Q_OBJECT

public:
    explicit Server(QObject *parent = nullptr);

    void setWatchPaths(QStringList paths);
    void resetWtachPaths(QStringList paths);
signals:
    void fileWrited(QStringList files);

private:
    void run() override;
    QStringList watchList;
    QMutex wlMutex;
};


#endif // SERVER_H
