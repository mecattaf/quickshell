qt_add_library(quickshell-webengine STATIC
	shellwebengineview.cpp
	shellbridge.cpp
	resources.qrc
)

qt_add_qml_module(quickshell-webengine
	URI Quickshell.WebEngine
	VERSION 0.2.1
	DEPENDENCIES QtQuick
	QML_FILES
		ShellChannel.qml
)

qs_add_module_deps_light(quickshell-webengine Quickshell)

install_qml_module(quickshell-webengine)

target_link_libraries(quickshell-webengine PRIVATE
	Qt::Quick
	Qt::QuickPrivate
	Qt::WebEngineQuick
	Qt::WebChannel
)

target_link_libraries(quickshell PRIVATE quickshell-webengineplugin)

qs_module_pch(quickshell-webengine SET large)
