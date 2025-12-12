#pragma once

#include <qcolor.h>
#include <qrect.h>
#include <qsgrendernode.h>

QT_FORWARD_DECLARE_CLASS(QRhiBuffer)
QT_FORWARD_DECLARE_CLASS(QRhiShaderResourceBindings)
QT_FORWARD_DECLARE_CLASS(QRhiGraphicsPipeline)

namespace qs::webengine {

/// QSGRenderNode that renders a tinted rounded rectangle using RHI.
/// This is the low-level rendering component used by MaterialSurface.
///
/// For POC 1, this renders a simple tinted rectangle with rounded corners
/// using SDF (Signed Distance Field) techniques in the fragment shader.
///
/// Future iterations will add:
/// - Multi-pass Kawase blur
/// - Backdrop texture sampling
/// - Vibrancy/saturation effects
class MaterialNode: public QSGRenderNode {
public:
	MaterialNode();
	~MaterialNode() override;

	// QSGRenderNode interface
	void render(const RenderState* state) override;
	void releaseResources() override;
	RenderingFlags flags() const override;
	QRectF rect() const override;
	StateFlags changedStates() const override;

	// Property setters
	void setRect(const QRectF& rect);
	void setTintColor(const QColor& color);
	void setCornerRadius(qreal radius);
	void setMaterialLevel(int level);
	void setOpacity(qreal opacity);

private:
	void initResources(const RenderState* state);

	// Geometry
	QRectF mRect;

	// Material properties
	QColor mTintColor = QColor(0, 0, 0, 64);
	qreal mCornerRadius = 12.0;
	int mMaterialLevel = 3;
	qreal mOpacity = 1.0;

	// RHI resources (created lazily in render())
	QRhiBuffer* mVertexBuffer = nullptr;
	QRhiBuffer* mUniformBuffer = nullptr;
	QRhiShaderResourceBindings* mResourceBindings = nullptr;
	QRhiGraphicsPipeline* mPipeline = nullptr;

	bool mResourcesInitialized = false;
};

} // namespace qs::webengine
