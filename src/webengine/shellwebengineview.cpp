#include "shellwebengineview.hpp"

#include <qcolor.h>
#include <qdebug.h>
#include <qlogging.h>
#include <qobject.h>

namespace qs::webengine {

ShellWebEngineView::ShellWebEngineView(QQuickItem* parent): QQuickWebEngineView(parent) {
	// Set transparent background BEFORE any URL is loaded
	// This is critical - transparency set after page load is silently ignored
	setBackgroundColor(Qt::transparent);

	// Connect visibility to lifecycle management
	connect(this, &QQuickItem::visibleChanged, this, [this]() {
		if (!mActive) return; // Don't auto-manage if manually set inactive
		updateLifecycleState();
	});
}

ShellWebEngineView::~ShellWebEngineView() {
	// Proper cleanup pattern for WebEngineView to minimize memory leaks
	// See: https://bugreports.qt.io/browse/QTBUG-50160
	setParent(nullptr);
	hide();

	// Setting lifecycleState to Discarded releases renderer resources
	setLifecycleState(QQuickWebEngineView::LifecycleState::Discarded);
}

void ShellWebEngineView::setActive(bool active) {
	if (mActive == active) return;
	mActive = active;
	updateLifecycleState();
	emit activeChanged();
}

void ShellWebEngineView::setDiscardWhenHidden(bool discard) {
	if (mDiscardWhenHidden == discard) return;
	mDiscardWhenHidden = discard;
	updateLifecycleState();
	emit discardWhenHiddenChanged();
}

void ShellWebEngineView::updateLifecycleState() {
	// Use Qt 6's lifecycle states to manage resource usage:
	// Active: Full functionality
	// Frozen: Suspended JS, minimal CPU, retains DOM (~50MB savings)
	// Discarded: Resources released, will auto-reload on reactivation (~200MB savings)

	if (mActive && isVisible()) {
		setLifecycleState(QQuickWebEngineView::LifecycleState::Active);
	} else if (mDiscardWhenHidden) {
		setLifecycleState(QQuickWebEngineView::LifecycleState::Discarded);
	} else {
		setLifecycleState(QQuickWebEngineView::LifecycleState::Frozen);
	}
}

} // namespace qs::webengine

