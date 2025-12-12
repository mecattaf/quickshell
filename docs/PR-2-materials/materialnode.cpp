#include "materialnode.hpp"

#include <qcolor.h>
#include <qfile.h>
#include <qmatrix4x4.h>
#include <qrect.h>
#include <qsgnode.h>
#include <qsgrendererinterface.h>

#include <private/qrhi_p.h>
#include <private/qrhirendertarget_p.h>
#include <private/qsgrendernode_p.h>

namespace qs::webengine {

// Uniform buffer layout (must match shader)
struct MaterialUniforms {
	float mvp[16];      // 4x4 matrix
	float tintColor[4]; // RGBA
	float params[4];    // x=cornerRadius, y=materialLevel, z=opacity, w=unused
	float size[2];      // width, height
	float padding[2];   // Alignment padding
};

static_assert(sizeof(MaterialUniforms) == 112, "MaterialUniforms size mismatch");

MaterialNode::MaterialNode() = default;

MaterialNode::~MaterialNode() {
	releaseResources();
}

void MaterialNode::releaseResources() {
	delete mPipeline;
	mPipeline = nullptr;

	delete mResourceBindings;
	mResourceBindings = nullptr;

	delete mUniformBuffer;
	mUniformBuffer = nullptr;

	delete mVertexBuffer;
	mVertexBuffer = nullptr;

	mResourcesInitialized = false;
}

QSGRenderNode::RenderingFlags MaterialNode::flags() const {
	// We blend with content behind us
	return QSGRenderNode::BoundedRectRendering | QSGRenderNode::DepthAwareRendering;
}

QRectF MaterialNode::rect() const {
	return mRect;
}

QSGRenderNode::StateFlags MaterialNode::changedStates() const {
	// We modify blend state
	return QSGRenderNode::BlendState;
}

void MaterialNode::setRect(const QRectF& rect) {
	mRect = rect;
}

void MaterialNode::setTintColor(const QColor& color) {
	mTintColor = color;
}

void MaterialNode::setCornerRadius(qreal radius) {
	mCornerRadius = radius;
}

void MaterialNode::setMaterialLevel(int level) {
	mMaterialLevel = qBound(1, level, 5);
}

void MaterialNode::setOpacity(qreal opacity) {
	mOpacity = qBound(0.0, opacity, 1.0);
}

void MaterialNode::initResources(const RenderState* state) {
	if (mResourcesInitialized) return;

	auto* rhi = state->rhi();
	if (!rhi) {
		qWarning() << "MaterialNode: No RHI available";
		return;
	}

	// Create vertex buffer for a fullscreen quad
	// Two triangles covering -1 to 1 in NDC
	static const float vertices[] = {
	    // Triangle 1
	    -1.0f, -1.0f,
	    1.0f, -1.0f,
	    1.0f, 1.0f,
	    // Triangle 2
	    -1.0f, -1.0f,
	    1.0f, 1.0f,
	    -1.0f, 1.0f,
	};

	mVertexBuffer = rhi->newBuffer(
	    QRhiBuffer::Immutable,
	    QRhiBuffer::VertexBuffer,
	    sizeof(vertices)
	);
	if (!mVertexBuffer->create()) {
		qWarning() << "MaterialNode: Failed to create vertex buffer";
		return;
	}

	// Create uniform buffer
	mUniformBuffer = rhi->newBuffer(
	    QRhiBuffer::Dynamic,
	    QRhiBuffer::UniformBuffer,
	    sizeof(MaterialUniforms)
	);
	if (!mUniformBuffer->create()) {
		qWarning() << "MaterialNode: Failed to create uniform buffer";
		return;
	}

	// Load shaders
	QShader vertexShader;
	QShader fragmentShader;

	{
		QFile vsFile(":/Quickshell/WebEngine/shaders/material.vert.qsb");
		if (vsFile.open(QIODevice::ReadOnly)) {
			vertexShader = QShader::fromSerialized(vsFile.readAll());
		} else {
			qWarning() << "MaterialNode: Failed to load vertex shader";
			return;
		}
	}

	{
		QFile fsFile(":/Quickshell/WebEngine/shaders/material.frag.qsb");
		if (fsFile.open(QIODevice::ReadOnly)) {
			fragmentShader = QShader::fromSerialized(fsFile.readAll());
		} else {
			qWarning() << "MaterialNode: Failed to load fragment shader";
			return;
		}
	}

	// Create shader resource bindings
	mResourceBindings = rhi->newShaderResourceBindings();
	mResourceBindings->setBindings({
	    QRhiShaderResourceBinding::uniformBuffer(
	        0,
	        QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
	        mUniformBuffer
	    ),
	});
	if (!mResourceBindings->create()) {
		qWarning() << "MaterialNode: Failed to create resource bindings";
		return;
	}

	// Create graphics pipeline
	mPipeline = rhi->newGraphicsPipeline();

	// Vertex input: 2D position
	QRhiVertexInputLayout inputLayout;
	inputLayout.setBindings({
	    QRhiVertexInputBinding{2 * sizeof(float)},
	});
	inputLayout.setAttributes({
	    QRhiVertexInputAttribute{0, 0, QRhiVertexInputAttribute::Float2, 0},
	});
	mPipeline->setVertexInputLayout(inputLayout);

	// Shaders
	mPipeline->setShaderStages({
	    QRhiShaderStage{QRhiShaderStage::Vertex, vertexShader},
	    QRhiShaderStage{QRhiShaderStage::Fragment, fragmentShader},
	});

	// Blending for transparency
	QRhiGraphicsPipeline::TargetBlend blend;
	blend.enable = true;
	blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
	blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
	blend.srcAlpha = QRhiGraphicsPipeline::One;
	blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
	mPipeline->setTargetBlends({blend});

	mPipeline->setShaderResourceBindings(mResourceBindings);
	mPipeline->setRenderPassDescriptor(state->renderPass());

	if (!mPipeline->create()) {
		qWarning() << "MaterialNode: Failed to create pipeline";
		return;
	}

	// Upload vertex data
	auto* resourceUpdates = rhi->nextResourceUpdateBatch();
	resourceUpdates->uploadStaticBuffer(mVertexBuffer, vertices);
	state->commandBuffer()->resourceUpdate(resourceUpdates);

	mResourcesInitialized = true;
}

void MaterialNode::render(const RenderState* state) {
	if (mRect.isEmpty()) return;

	// Initialize resources if needed (must be done in render context)
	if (!mResourcesInitialized) {
		initResources(state);
	}

	if (!mResourcesInitialized || !mPipeline) return;

	auto* rhi = state->rhi();
	auto* cb = state->commandBuffer();

	// Update uniform buffer
	MaterialUniforms uniforms{};

	// Calculate MVP matrix
	QMatrix4x4 mvp = *state->projectionMatrix() * *matrix();

	// Scale and translate to render at correct position
	mvp.translate(mRect.x() + mRect.width() / 2, mRect.y() + mRect.height() / 2);
	mvp.scale(mRect.width() / 2, mRect.height() / 2);

	memcpy(uniforms.mvp, mvp.constData(), 16 * sizeof(float));

	// Tint color (premultiplied alpha)
	float alpha = mTintColor.alphaF();
	uniforms.tintColor[0] = mTintColor.redF() * alpha;
	uniforms.tintColor[1] = mTintColor.greenF() * alpha;
	uniforms.tintColor[2] = mTintColor.blueF() * alpha;
	uniforms.tintColor[3] = alpha;

	// Parameters
	// Normalize corner radius to be relative to size
	float normalizedRadius = static_cast<float>(mCornerRadius * 2.0 / qMin(mRect.width(), mRect.height()));
	uniforms.params[0] = normalizedRadius;
	uniforms.params[1] = static_cast<float>(mMaterialLevel);
	uniforms.params[2] = static_cast<float>(mOpacity);
	uniforms.params[3] = 0.0f; // unused

	uniforms.size[0] = static_cast<float>(mRect.width());
	uniforms.size[1] = static_cast<float>(mRect.height());
	uniforms.padding[0] = 0.0f;
	uniforms.padding[1] = 0.0f;

	auto* resourceUpdates = rhi->nextResourceUpdateBatch();
	resourceUpdates->updateDynamicBuffer(mUniformBuffer, 0, sizeof(uniforms), &uniforms);
	cb->resourceUpdate(resourceUpdates);

	// Render
	cb->setGraphicsPipeline(mPipeline);
	cb->setShaderResources(mResourceBindings);

	const QRhiCommandBuffer::VertexInput vertexInput(mVertexBuffer, 0);
	cb->setVertexInput(0, 1, &vertexInput);
	cb->draw(6); // 6 vertices for 2 triangles
}

} // namespace qs::webengine
