#pragma once

#include <qcolor.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qquickitem.h>
#include <qtmetamacros.h>

namespace qs::webengine {

///! A surface that renders material/glass effects beneath web content.
/// MaterialSurface creates a visual layer that supports blur, tint, and
/// rounded corner effects. Position it beneath a ShellWebEngineView with
/// `z: -1` to create glass-behind-content effects.
///
/// For POC 1, this renders a simple tinted rounded rectangle.
/// Future versions will support multi-pass blur effects.
///
/// Example:
/// ```qml
/// Item {
///     MaterialSurface {
///         anchors.fill: parent
///         z: -1
///         materialLevel: 3
///         tintColor: "#40000000"
///         cornerRadius: 12
///     }
///     ShellWebEngineView {
///         anchors.fill: parent
///         url: "panel.html"
///     }
/// }
/// ```
class MaterialSurface: public QQuickItem {
	Q_OBJECT;
	QML_ELEMENT;
	// clang-format off
	/// Material intensity level from 1 (subtle) to 5 (strong).
	/// Higher levels produce more prominent tinting/blur effects.
	Q_PROPERTY(int materialLevel READ materialLevel WRITE setMaterialLevel NOTIFY materialLevelChanged);
	/// Tint color applied to the material surface.
	/// Use colors with alpha for translucent effects (e.g., "#40000000").
	Q_PROPERTY(QColor tintColor READ tintColor WRITE setTintColor NOTIFY tintColorChanged);
	/// Corner radius for rounded edges.
	Q_PROPERTY(qreal cornerRadius READ cornerRadius WRITE setCornerRadius NOTIFY cornerRadiusChanged);
	/// Opacity of the material surface (0.0 = fully transparent, 1.0 = fully opaque).
	Q_PROPERTY(qreal opacity READ materialOpacity WRITE setMaterialOpacity NOTIFY materialOpacityChanged);
	// clang-format on

public:
	explicit MaterialSurface(QQuickItem* parent = nullptr);
	~MaterialSurface() override = default;

	[[nodiscard]] int materialLevel() const { return mMaterialLevel; }
	void setMaterialLevel(int level);

	[[nodiscard]] QColor tintColor() const { return mTintColor; }
	void setTintColor(const QColor& color);

	[[nodiscard]] qreal cornerRadius() const { return mCornerRadius; }
	void setCornerRadius(qreal radius);

	[[nodiscard]] qreal materialOpacity() const { return mOpacity; }
	void setMaterialOpacity(qreal opacity);

signals:
	void materialLevelChanged();
	void tintColorChanged();
	void cornerRadiusChanged();
	void materialOpacityChanged();

protected:
	QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;

private:
	int mMaterialLevel = 3;
	QColor mTintColor = QColor(0, 0, 0, 64); // #40000000
	qreal mCornerRadius = 12.0;
	qreal mOpacity = 1.0;
};

} // namespace qs::webengine
