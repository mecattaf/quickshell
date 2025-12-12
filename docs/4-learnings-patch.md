I had previously tried patching quickshell rather than forking the codebase and modifying it. It was not sufficient to get my desired functionality working. I still think it would be useful to provide those items here for reference, may be helpful when packaging my own quickshell (for testing).

For the git rolling release:

quickshell-webengine-git.patch:
```

--- a/CMakeLists.txt
+++ b/CMakeLists.txt
@@ -46,6 +46,7 @@ boption(FRAME_POINTERS "Keep Frame Pointers (dev)" ${ASAN})
 
 boption(CRASH_REPORTER "Crash Handling" ON)
 boption(USE_JEMALLOC "Use jemalloc" ON)
+boption(WEBENGINE "QtWebEngine" ON)
 boption(SOCKETS "Unix Sockets" ON)
 boption(WAYLAND "Wayland" ON)
 boption(WAYLAND_WLR_LAYERSHELL "  Wlroots Layer-Shell" ON REQUIRES WAYLAND)
@@ -121,6 +122,11 @@ if (DBUS)
 	list(APPEND QT_FPDEPS DBus)
 endif()
 
+# WebEngine support (optional)
+if (WEBENGINE_effective)
+	list(APPEND QT_FPDEPS WebEngineQuick WebChannel)
+endif()
+
 find_package(Qt6 REQUIRED COMPONENTS ${QT_FPDEPS})
 
 # In Qt 6.10, private dependencies are required to be explicit,
@@ -146,6 +152,12 @@ if (USE_JEMALLOC)
 	pkg_check_modules(JEMALLOC REQUIRED jemalloc)
 	target_link_libraries(quickshell PRIVATE ${JEMALLOC_LIBRARIES})
 endif()
+
+# WebEngine linking
+if (WEBENGINE_effective)
+	target_link_libraries(quickshell PRIVATE Qt6::WebEngineQuick Qt6::WebChannel)
+	target_compile_definitions(quickshell PRIVATE QS_WEBENGINE)
+endif()
 
 install(CODE "
 	execute_process(
--- a/src/launch/launch.cpp
+++ b/src/launch/launch.cpp
@@ -26,6 +26,10 @@
 #include "build.hpp"
 #include "launch_p.hpp"
 
+#ifdef QS_WEBENGINE
+#include <QtWebEngineQuick/qtwebenginequickglobal.h>
+#endif
+
 #if CRASH_REPORTER
 #include "../crash/handler.hpp"
 #endif
@@ -186,6 +190,13 @@ int launch(const LaunchArgs& args, char** argv, QCoreApplication* coreApplicati
 
 	delete coreApplication;
 
+#ifdef QS_WEBENGINE
+	// Initialize QtWebEngine after QCoreApplication is deleted,
+	// but before QGuiApplication is created (Qt requirement)
+	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
+	QtWebEngineQuick::initialize();
+#endif
+
 	QGuiApplication* app = nullptr;
 	auto qArgC = 0;
```

quickshell-webengine-git.spec:
```

%bcond_with         asan

%global commit      e9bad67619ee9937a1bbecfc6ad3b4231d2ecdc3
%global commits     709
%global snapdate    20251125
%global tag         0.2.1

Name:               quickshell-webengine-git
Version:            %{tag}^%{commits}.git%(c=%{commit}; echo ${c:0:7})
Release:            1%{?dist}
Summary:            QtQuick desktop shell toolkit (Git snapshot) with QtWebEngine support

License:            LGPL-3.0-only AND GPL-3.0-only
URL:                https://github.com/quickshell-mirror/quickshell
Source0:            %{url}/archive/%{commit}/quickshell-%{commit}.tar.gz
Patch0:             quickshell-webengine-git.patch

Conflicts:          quickshell-git <= %{tag}
Conflicts:          quickshell-webengine
Conflicts:          quickshell
Provides:           desktop-notification-daemon

%if 0%{?fedora} >= 43
BuildRequires:      breakpad-static
%endif
BuildRequires:      cmake
BuildRequires:      cmake(Qt6Core)
BuildRequires:      cmake(Qt6Qml)
BuildRequires:      cmake(Qt6ShaderTools)
BuildRequires:      cmake(Qt6WaylandClient)
BuildRequires:      cmake(Qt6WebEngineQuick)
BuildRequires:      cmake(Qt6WebChannel)
BuildRequires:      gcc-c++
BuildRequires:      ninja-build
BuildRequires:      pkgconfig(breakpad)
BuildRequires:      pkgconfig(CLI11)
BuildRequires:      pkgconfig(gbm)
BuildRequires:      pkgconfig(glib-2.0)
BuildRequires:      pkgconfig(jemalloc)
BuildRequires:      pkgconfig(libdrm)
BuildRequires:      pkgconfig(libpipewire-0.3)
BuildRequires:      pkgconfig(pam)
BuildRequires:      pkgconfig(polkit-agent-1)
BuildRequires:      pkgconfig(polkit-gobject-1)
BuildRequires:      pkgconfig(wayland-client)
BuildRequires:      pkgconfig(wayland-protocols)
BuildRequires:      qt6-qtbase-private-devel
BuildRequires:      spirv-tools
# Enable ASAN if bcond is active
%if %{with asan}
BuildRequires:      libasan
%endif

Requires:           qt6-qtwebengine
Requires:           qt6-qtwebchannel

%description
Flexible toolkit for building desktop shells with QtQuick.

This variant is a Git snapshot and includes support for QtWebEngine
and QtWebChannel.

%prep
%autosetup -n quickshell-%{commit} -p1

%build
%cmake -GNinja \
%if %{with asan}
      -DASAN=ON \
%endif
      -DBUILD_SHARED_LIBS=OFF \
      -DCMAKE_BUILD_TYPE=Release \
      -DDISTRIBUTOR="Fedora COPR (mecattaf/packages)" \
      -DDISTRIBUTOR_DEBUGINFO_AVAILABLE=YES \
      -DGIT_REVISION=%{commit} \
      -DINSTALL_QML_PREFIX=%{_lib}/qt6/qml \
      -DWEBENGINE=ON

%cmake_build

%install
%cmake_install

%files
%license LICENSE
%license LICENSE-GPL
%doc BUILD.md
%doc CONTRIBUTING.md
%doc README.md
%doc changelog/v%{tag}.md
%{_bindir}/qs
%{_bindir}/quickshell
%{_datadir}/applications/org.quickshell.desktop
%{_datadir}/icons/hicolor/scalable/apps/org.quickshell.svg
%{_libdir}/qt6/qml/Quickshell

%changelog
* Thu Nov 28 2024 Your Name <you@example.com> - %{tag}^%{commits}.git-1
- Add QtWebEngine support for Git snapshot
```
