TARGET = everylauncher-monitor
TEMPLATE = app
CONFIG += c++11
QT += dbus
QT -= gui


SOURCES += \
    main.cpp \
    server.cpp\


#CONFIG(debug, debug|release) {
#    QMAKE_RPATHLINKDIR += $$_PRO_FILE_PWD_/../../../library/bin/debug
#} else {
#    QMAKE_RPATHLINKDIR += $$_PRO_FILE_PWD_/../../../library/bin/release
#}

LIBS += -lrecoll -L/usr/lib/recoll
INCLUDEPATH += ../dirvers\
                ../../recoll1-code/src/common\
                ../../recoll1-code/src/utils
#LIBS += -L$$OUT_PWD/../../lib -ldeepin-anything-server-lib

#CONFIG(debug, debug|release) {
#    DEPENDPATH += $$OUT_PWD/../../lib
#    unix:QMAKE_RPATHDIR += $$OUT_PWD/../../lib
#}

HEADERS += \
    server.h

isEmpty(PREFIX): PREFIX = /usr

dbus.files = $$PWD/com.gitee.wanywhn.EveryLauncherMonitor.xml
dbus.header_flags += -l Server -i $$PWD/server.h
dbus.source_flags += -l Server
DBUS_ADAPTORS += dbus

dbus_itface.files= $$PWD/com.gitee.wanywhn.EveryLauncherMonitor.xml
dbus_itface.header_flags += -c EveryLauncherMonitorInterface
dbus_itface.source_flags += -c EveryLauncherMonitorInterface
DBUS_INTERFACES+=dbus_itface

dbus_xmls.path = /usr/share/dbus-1/interfaces
dbus_xmls.files = $$dbus.files
#
#dbus_service.path = /usr/share/dbus-1/system-services
#dbus_service.files = $$PWD/everylauncher-monitor.service
#
#dbus_config.path = /etc/dbus-1/system.d
#dbus_config.files = $$PWD/com.gitee.wanywhn.everylauncher.conf

target.path = $$PREFIX/bin

systemd_service.files = $${TARGET}.service
systemd_service.path = /lib/systemd/system

sysusers.files = systemd.sysusers.d/$${TARGET}.conf
sysusers.path = $$PREFIX/lib/sysusers.d

INSTALLS += target systemd_service sysusers dbus_xmls #dbus_service dbus_config