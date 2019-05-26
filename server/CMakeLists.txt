cmake_minimum_required(VERSION 3.11)
project(everylauncher-monitor VERSION 1.0.0)

find_package(Qt5Gui)
find_package(Qt5DBus)
set(CMAKE_AUTOMOC ON)

set(dbus_interface_xml "com.gitee.wanywhn.EveryLauncherMonitor.xml")
## FIXME why not work
qt5_generate_dbus_interface(
		${CMAKE_CURRENT_SOURCE_DIR}}/server.h
		${dbus_interface_xml}
		OPTIONS -a

)


qt5_add_dbus_interface(
        ITF
		${dbus_interface_xml}
		everylaunchermonitor_interface
		#        EveryLauncherInterface
)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC)
qt5_add_dbus_adaptor(
        SRC
		${dbus_interface_xml}
		${CMAKE_CURRENT_SOURCE_DIR}/server.h
		Server
		everylaunchermonitor_adaptor
		EveryLauncherMonitorAdaptor
)

add_executable(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME}
		PRIVATE
		../../recoll1-code/src/common
		../../recoll1-code/src/utils
		${CMAKE_CURRENT_BINARY_DIR}
		)
target_link_libraries(${PROJECT_NAME} PUBLIC Qt5::Gui Qt5::DBus recoll)
target_link_directories(${PROJECT_NAME} PRIVATE /usr/lib/recoll)

INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION bin
	)

INSTALL(FILES com.gitee.wanywhn.EveryLauncherMonitor.xml
	DESTINATION /usr/share/dbus-1/interfaces
	)
INSTALL(FILES com.gitee.wanywhn.EveryLauncherMonitor.service
	DESTINATION /usr/share/dbus-1/services
	)
INSTALL(FILES ${PROJECT_NAME}.service
	DESTINATION /usr/lib/systemd/user
	)







SET(CPACK_DEBIAN_PACKAGE_DEPENDS "recollcmd")

#SET(CMAKE_DEBIAN_PACKAGE_ARCHITECTUREE "i386")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "一个全文搜索加桌面启动器的服务端。")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Jia Qingtong (wanywhn@qq.com)")
#set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
#	"${CMAKE_CURRENT_SOURCE_DIR}/postinst"
#	)
INCLUDE(CPack)

