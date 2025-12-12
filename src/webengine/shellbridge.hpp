#pragma once

#include <qcolor.h>
#include <qhash.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qvariant.h>

namespace qs::webengine {

///! Bridge object for QtWebChannel communication between C++ and JavaScript.
/// ShellBridge provides the API that JavaScript code can call via QtWebChannel.
/// Register it with a WebChannel to expose shell functionality to web content.
///
/// On the JavaScript side, access via:
/// ```javascript
/// new QWebChannel(qt.webChannelTransport, function(channel) {
///     var shell = channel.objects.shell;
///     shell.log("info", "Connected to shell");
///     console.log("Dark mode:", shell.darkMode);
/// });
/// ```
///
/// > [!NOTE] All method calls from JavaScript are asynchronous.
/// > Use callbacks or connect to signals for return values.
class ShellBridge: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	// clang-format off
	/// Whether the system is in dark mode.
	Q_PROPERTY(bool darkMode READ darkMode NOTIFY darkModeChanged);
	/// The current accent color as a hex string (e.g., "#007AFF").
	Q_PROPERTY(QString accentColor READ accentColor NOTIFY accentColorChanged);
	/// List of registered material regions for dynamic MaterialSurface creation.
	/// Each region is a QVariantMap with: id, x, y, width, height, materialLevel, cornerRadius.
	Q_PROPERTY(QVariantList materialRegions READ materialRegions NOTIFY materialRegionsChanged);
	// clang-format on

public:
	explicit ShellBridge(QObject* parent = nullptr);
	~ShellBridge() override = default;

	[[nodiscard]] bool darkMode() const { return mDarkMode; }
	[[nodiscard]] QString accentColor() const { return mAccentColor; }
	[[nodiscard]] QVariantList materialRegions() const;

	/// Log a message to Quickshell's console.
	/// @param level One of: "debug", "info", "warn", "error"
	/// @param message The message to log
	Q_INVOKABLE void log(const QString& level, const QString& message);

	/// Get all theme tokens as a JSON-compatible object.
	/// Returns colors, spacing, and radius values for consistent theming.
	Q_INVOKABLE QVariantMap getThemeTokens();

	/// Register a material region for blur effects beneath web content.
	/// @param regionData Object with: id, x, y, width, height, materialLevel, cornerRadius
	Q_INVOKABLE void registerMaterialRegion(const QVariantMap& regionData);

	/// Unregister a previously registered material region.
	/// @param id The region identifier to remove
	Q_INVOKABLE void unregisterMaterialRegion(const QString& id);

	/// Update an existing material region's properties.
	/// @param id The region identifier
	/// @param updates Object with properties to update
	Q_INVOKABLE void updateMaterialRegion(const QString& id, const QVariantMap& updates);

	// Called from C++ side to update theme state
	void setDarkMode(bool dark);
	void setAccentColor(const QString& color);

signals:
	void darkModeChanged();
	void accentColorChanged();
	/// Emitted when a material region is added, removed, or updated.
	void materialRegionsChanged();

private:
	bool mDarkMode = false;
	QString mAccentColor = "#007AFF";
	QHash<QString, QVariantMap> mMaterialRegions;
};

} // namespace qs::webengine
