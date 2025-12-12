#include "shellbridge.hpp"

#include <qdebug.h>
#include <qlogging.h>
#include <qvariant.h>

namespace qs::webengine {

ShellBridge::ShellBridge(QObject* parent): QObject(parent) {
	// TODO: Connect to Quickshell's actual theme system when available
	// For now, use sensible defaults
}

void ShellBridge::log(const QString& level, const QString& message) {
	// Route web console messages to Qt's logging system
	if (level == "debug") {
		qDebug().noquote() << "[WebEngine]" << message;
	} else if (level == "info") {
		qInfo().noquote() << "[WebEngine]" << message;
	} else if (level == "warn") {
		qWarning().noquote() << "[WebEngine]" << message;
	} else if (level == "error") {
		qCritical().noquote() << "[WebEngine]" << message;
	} else {
		qInfo().noquote() << "[WebEngine:" << level << "]" << message;
	}
}

QVariantMap ShellBridge::getThemeTokens() {
	QVariantMap tokens;

	// Color tokens
	QVariantMap colors;
	colors["accent"] = mAccentColor;
	colors["background"] = mDarkMode ? "#1C1C1E" : "#F2F2F7";
	colors["surface"] = mDarkMode ? "#2C2C2E" : "#FFFFFF";
	colors["text"] = mDarkMode ? "#FFFFFF" : "#000000";
	colors["textSecondary"] = mDarkMode ? "#8E8E93" : "#6C6C70";
	colors["separator"] = mDarkMode ? "#38383A" : "#C6C6C8";
	tokens["colors"] = colors;

	// Spacing tokens (in pixels)
	QVariantMap spacing;
	spacing["xs"] = 4;
	spacing["sm"] = 8;
	spacing["md"] = 16;
	spacing["lg"] = 24;
	spacing["xl"] = 32;
	tokens["spacing"] = spacing;

	// Radius tokens
	QVariantMap radius;
	radius["sm"] = 4;
	radius["md"] = 8;
	radius["lg"] = 12;
	radius["xl"] = 16;
	radius["full"] = 9999;
	tokens["radius"] = radius;

	// Mode
	tokens["darkMode"] = mDarkMode;

	return tokens;
}

QVariantList ShellBridge::materialRegions() const {
	return mMaterialRegions.values();
}

void ShellBridge::registerMaterialRegion(const QVariantMap& regionData) {
	QString id = regionData.value("id").toString();
	if (id.isEmpty()) {
		qWarning() << "registerMaterialRegion: region must have an 'id' field";
		return;
	}

	mMaterialRegions[id] = regionData;
	emit materialRegionsChanged();
}

void ShellBridge::unregisterMaterialRegion(const QString& id) {
	if (mMaterialRegions.remove(id) > 0) {
		emit materialRegionsChanged();
	}
}

void ShellBridge::updateMaterialRegion(const QString& id, const QVariantMap& updates) {
	if (!mMaterialRegions.contains(id)) {
		qWarning() << "updateMaterialRegion: unknown region id" << id;
		return;
	}

	auto& region = mMaterialRegions[id];
	for (auto it = updates.begin(); it != updates.end(); ++it) {
		region[it.key()] = it.value();
	}

	emit materialRegionsChanged();
}

void ShellBridge::setDarkMode(bool dark) {
	if (mDarkMode == dark) return;
	mDarkMode = dark;
	emit darkModeChanged();
}

void ShellBridge::setAccentColor(const QString& color) {
	if (mAccentColor == color) return;
	mAccentColor = color;
	emit accentColorChanged();
}

} // namespace qs::webengine
