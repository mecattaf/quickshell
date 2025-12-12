#pragma once

#include <qcolor.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qurl.h>
#include <QtWebEngineQuick/qquickwebengineview.h>
#include <QtWebChannel/qwebchannel.h>

namespace qs::webengine {

///! WebEngineView optimized for desktop shell usage.
/// A wrapper around QtWebEngine's WebEngineView with shell-optimized defaults:
/// - Transparent background enabled by default
/// - Lifecycle state management (Active/Frozen/Discarded)
/// - Automatic visibility-based resource management
///
/// > [!IMPORTANT] You must add `//@ pragma EnableWebEngine` at the top of your
/// > shell.qml file to use WebEngine components.
///
/// > [!WARNING] Do NOT set `layer.enabled: true` on this component unless
/// > absolutely necessary — it significantly impacts performance and memory.
///
/// > [!TIP] For DevTools debugging, set the environment variable
/// > `QS_WEBENGINE_DEVTOOLS_PORT=9222` before launching quickshell.
///
/// Example usage:
/// ```qml
/// ShellWebEngineView {
///     url: "file:///path/to/panel.html"
///     webChannel: myChannel
/// }
/// ```
class ShellWebEngineView: public QQuickWebEngineView {
	Q_OBJECT;
	QML_ELEMENT;
	// clang-format off
	/// Whether the view is currently active. When false, the view enters a frozen state
	/// to reduce CPU usage while hidden.
	Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged);
	/// When true, hidden views enter Discarded state (releases most resources, will reload).
	/// When false (default), hidden views enter Frozen state (suspends JS, retains DOM).
	/// Use discardWhenHidden for views that are rarely shown to save ~200MB RAM each.
	Q_PROPERTY(bool discardWhenHidden READ discardWhenHidden WRITE setDiscardWhenHidden NOTIFY discardWhenHiddenChanged);
	// clang-format on

public:
	explicit ShellWebEngineView(QQuickItem* parent = nullptr);
	~ShellWebEngineView() override;

	[[nodiscard]] bool active() const { return mActive; }
	void setActive(bool active);

	[[nodiscard]] bool discardWhenHidden() const { return mDiscardWhenHidden; }
	void setDiscardWhenHidden(bool discard);

signals:
	void activeChanged();
	void discardWhenHiddenChanged();

private:
	void updateLifecycleState();

	bool mActive = true;
	bool mDiscardWhenHidden = false;
};

} // namespace qs::webengine

