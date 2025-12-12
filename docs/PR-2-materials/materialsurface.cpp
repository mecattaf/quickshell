#include "materialsurface.hpp"
#include "materialnode.hpp"

#include <qcolor.h>
#include <qquickitem.h>
#include <qsgnode.h>

namespace qs::webengine {

MaterialSurface::MaterialSurface(QQuickItem* parent): QQuickItem(parent) {
	// Enable custom rendering
	setFlag(QQuickItem::ItemHasContents, true);
}

void MaterialSurface::setMaterialLevel(int level) {
	level = qBound(1, level, 5);
	if (mMaterialLevel == level) return;
	mMaterialLevel = level;
	update();
	emit materialLevelChanged();
}

void MaterialSurface::setTintColor(const QColor& color) {
	if (mTintColor == color) return;
	mTintColor = color;
	update();
	emit tintColorChanged();
}

void MaterialSurface::setCornerRadius(qreal radius) {
	if (qFuzzyCompare(mCornerRadius, radius)) return;
	mCornerRadius = radius;
	update();
	emit cornerRadiusChanged();
}

void MaterialSurface::setMaterialOpacity(qreal opacity) {
	opacity = qBound(0.0, opacity, 1.0);
	if (qFuzzyCompare(mOpacity, opacity)) return;
	mOpacity = opacity;
	update();
	emit materialOpacityChanged();
}

QSGNode* MaterialSurface::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* /*data*/) {
	auto* node = static_cast<MaterialNode*>(oldNode);

	if (width() <= 0 || height() <= 0) {
		delete node;
		return nullptr;
	}

	if (!node) {
		node = new MaterialNode();
	}

	// Update node with current properties
	node->setRect(QRectF(0, 0, width(), height()));
	node->setTintColor(mTintColor);
	node->setCornerRadius(mCornerRadius);
	node->setMaterialLevel(mMaterialLevel);
	node->setOpacity(mOpacity);

	node->markDirty(QSGNode::DirtyMaterial | QSGNode::DirtyGeometry);

	return node;
}

} // namespace qs::webengine
