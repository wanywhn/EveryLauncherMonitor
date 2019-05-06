/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp -l Server -i /home/tender/workplace/git/everylauncher-server/server/server.h -a everylaunchermonitor_adaptor.h: com.gitee.wanywhn.EveryLauncherMonitor.xml
 *
 * qdbusxml2cpp is Copyright (C) 2017 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#ifndef EVERYLAUNCHERMONITOR_ADAPTOR_H
#define EVERYLAUNCHERMONITOR_ADAPTOR_H

#include <QtCore/QObject>
#include <QtDBus/QtDBus>
#include "/home/tender/workplace/git/everylauncher-server/server/server.h"
QT_BEGIN_NAMESPACE
class QByteArray;
template<class T> class QList;
template<class Key, class Value> class QMap;
class QString;
class QStringList;
class QVariant;
QT_END_NAMESPACE

/*
 * Adaptor class for interface com.gitee.wanywhn.EveryLauncherMonitor
 */
class EveryLauncherMonitorAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.gitee.wanywhn.EveryLauncherMonitor")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"com.gitee.wanywhn.EveryLauncherMonitor\">\n"
"    <signal name=\"fileWrited\">\n"
"      <arg direction=\"out\" type=\"as\" name=\"files\"/>\n"
"    </signal>\n"
"    <method name=\"myrun\"/>\n"
"    <method name=\"setWatchPaths\">\n"
"      <arg direction=\"in\" type=\"as\" name=\"paths\"/>\n"
"    </method>\n"
"    <method name=\"setFileMonitorInter\">\n"
"      <arg direction=\"in\" type=\"i\" name=\"sec\"/>\n"
"    </method>\n"
"  </interface>\n"
        "")
public:
    EveryLauncherMonitorAdaptor(Server *parent);
    virtual ~EveryLauncherMonitorAdaptor();

    inline Server *parent() const
    { return static_cast<Server *>(QObject::parent()); }

public: // PROPERTIES
public Q_SLOTS: // METHODS
    void myrun();
    void setFileMonitorInter(int sec);
    void setWatchPaths(const QStringList &paths);
Q_SIGNALS: // SIGNALS
    void fileWrited(const QStringList &files);
};

#endif
