#include "TextLabel.h"
#include "TextEncoding.h"

#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include "Graphics/Resource/TextureManager.h"
#include "Math/MathUtils.h"
#include "Platform/WindowManager.h"

#include <algorithm>
#include <cassert>
#include <vector>

namespace {
	/// 四角形1つぶんの頂点数とインデックス数
	constexpr uint32_t kVertexPerGlyph = 4;
	constexpr uint32_t kIndexPerGlyph = 6;

	/// バッファを作り直す回数を減らすため、この単位で切り上げて確保する
	constexpr uint32_t kCapacityGranularity = 32;
}

TextLabel::TextLabel(const std::string& name, Font* font, SpriteLayer layer)
	: name_(name), font_(font), layer_(layer) {
	CreateConstantBuffers();
}

TextLabel::~TextLabel() {
	// SpriteManager の終了処理より後に消えることは無いが、
	// 念のため解放先が生きているかを見てから返す
	DirectXManager* dxManager = SpriteManager::GetInstance().GetDxManager();
	if (!dxManager) return;

	ResourceManager* resourceManager = dxManager->GetResourceManager();
	if (hasBuffers_) {
		resourceManager->ReleaseBuffer(vertexHandle_);
		resourceManager->ReleaseBuffer(indexHandle_);
	}
	resourceManager->ReleaseBuffer(materialHandle_);
	resourceManager->ReleaseBuffer(transformHandle_);
	resourceManager->ReleaseBuffer(shadowMaterialHandle_);
	resourceManager->ReleaseBuffer(shadowTransformHandle_);
}

void TextLabel::SetText(const std::string& text) {
	if (text_ == text) return;

	text_ = text;
	codePoints_ = DecodeUtf8(text_);
	isDirty_ = true;
}

void TextLabel::SetFontSize(float pixelSize) {
	if (fontSize_ == pixelSize) return;

	fontSize_ = pixelSize;
	isDirty_ = true;
}

void TextLabel::SetAlign(TextAlignX alignX, TextAlignY alignY) {
	if (alignX_ == alignX && alignY_ == alignY) return;

	alignX_ = alignX;
	alignY_ = alignY;
	isDirty_ = true;
}

void TextLabel::SetLineSpacing(float scale) {
	if (lineSpacing_ == scale) return;

	lineSpacing_ = scale;
	isDirty_ = true;
}

void TextLabel::SetShadow(bool enabled, const Vector2& offset, const Vector4& color) {
	hasShadow_ = enabled;
	shadowOffset_ = offset;
	shadowColor_ = color;
}

void TextLabel::Update() {
	if (isDirty_) {
		BuildVertices();
		isDirty_ = false;
	}

	// スプライトと同じ組み方。文字の大きさは頂点側へ焼いてあるので、ここでは位置だけ動かす
	const Matrix4x4 projectionMatrix = MakeOrthographicMatrix(
		0.0f, 0.0f, float(WindowManager::kGameWidth), float(WindowManager::kGameHeight), 0.0f, 100.0f);

	// 回転にはクォータニオンを取る版もあるので、オイラー角側だと分かるように型を書く
	const Matrix4x4 worldMatrix = MakeAffineMatrix(
		Vector3{ 1.0f, 1.0f, 1.0f }, Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ position_.x, position_.y, 0.0f });

	transformData_->World = worldMatrix;
	transformData_->WVP = worldMatrix * projectionMatrix;
	materialData_->color = color_;

	if (hasShadow_) {
		const Matrix4x4 shadowWorld = MakeAffineMatrix(
			Vector3{ 1.0f, 1.0f, 1.0f }, Vector3{ 0.0f, 0.0f, 0.0f },
			Vector3{ position_.x + shadowOffset_.x, position_.y + shadowOffset_.y, 0.0f });

		shadowTransformData_->World = shadowWorld;
		shadowTransformData_->WVP = shadowWorld * projectionMatrix;
		// 影は本体のアルファに追従させる。フェード中に影だけ残らないようにするため
		shadowMaterialData_->color = { shadowColor_.x, shadowColor_.y, shadowColor_.z, shadowColor_.w * color_.w };
	}
}

void TextLabel::Draw() {
	if (!hasBuffers_ || drawGlyphCount_ == 0 || !font_) return;

	DirectXManager* dxManager = SpriteManager::GetInstance().GetDxManager();
	ResourceManager* resourceManager = dxManager->GetResourceManager();
	ID3D12GraphicsCommandList* commandList = dxManager->GetCommandList();

	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	commandList->IASetIndexBuffer(&indexBufferView_);
	commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance().GetSrvHandleGPU(font_->GetAtlasTextureName()));
	commandList->SetGraphicsRootDescriptorTable(3, TextureManager::GetInstance().GetDissolveNoiseSrvHandleGPU());

	const UINT indexCount = drawGlyphCount_ * kIndexPerGlyph;

	// 影を先に、同じ頂点のまま位置と色だけ変えて描く
	if (hasShadow_) {
		commandList->SetGraphicsRootConstantBufferView(0, resourceManager->GetGPUVirtualAddress(shadowMaterialHandle_));
		commandList->SetGraphicsRootConstantBufferView(1, resourceManager->GetGPUVirtualAddress(shadowTransformHandle_));
		commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
	}

	commandList->SetGraphicsRootConstantBufferView(0, resourceManager->GetGPUVirtualAddress(materialHandle_));
	commandList->SetGraphicsRootConstantBufferView(1, resourceManager->GetGPUVirtualAddress(transformHandle_));
	commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}

void TextLabel::BuildVertices() {
	measuredSize_ = { 0.0f, 0.0f };
	drawGlyphCount_ = 0;

	if (!font_ || codePoints_.empty()) return;

	// 改行で行に割る
	std::vector<std::u32string> lines;
	lines.emplace_back();
	for (char32_t codePoint : codePoints_) {
		if (codePoint == U'\n') {
			lines.emplace_back();
			continue;
		}
		if (codePoint == U'\r') continue;
		lines.back().push_back(codePoint);
	}

	// フォントを焼いた大きさと、表示したい大きさの比
	const float scale = (font_->GetPixelHeight() > 0.0f) ? (fontSize_ / font_->GetPixelHeight()) : 1.0f;
	const float lineHeight = font_->GetLineHeight() * scale * lineSpacing_;

	std::vector<float> lineWidths;
	lineWidths.reserve(lines.size());
	float maxWidth = 0.0f;
	for (const std::u32string& line : lines) {
		const float width = MeasureLineWidth(line) * scale;
		lineWidths.push_back(width);
		maxWidth = (std::max)(maxWidth, width);
	}

	const float blockHeight = lineHeight * lines.size();
	measuredSize_ = { maxWidth, blockHeight };

	EnsureCapacity(static_cast<uint32_t>(codePoints_.size()));
	if (!hasBuffers_) return;

	float originY = 0.0f;
	switch (alignY_) {
	case TextAlignY::Middle: originY = -blockHeight * 0.5f; break;
	case TextAlignY::Bottom: originY = -blockHeight; break;
	default: break;
	}

	uint32_t glyphIndex = 0;
	for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
		const std::u32string& line = lines[lineIndex];

		float pen = 0.0f;
		switch (alignX_) {
		case TextAlignX::Center: pen = -lineWidths[lineIndex] * 0.5f; break;
		case TextAlignX::Right: pen = -lineWidths[lineIndex]; break;
		default: break;
		}

		const float baseline = originY + font_->GetAscent() * scale + lineHeight * lineIndex;

		for (size_t charIndex = 0; charIndex < line.size(); ++charIndex) {
			const FontGlyph* glyph = font_->GetGlyph(line[charIndex]);
			// アトラスが満杯で焼けなかった文字は飛ばす
			if (!glyph) continue;

			if (glyph->size.x > 0.0f && glyph->size.y > 0.0f) {
				const float left = pen + glyph->bearing.x * scale;
				const float top = baseline + glyph->bearing.y * scale;
				const float right = left + glyph->size.x * scale;
				const float bottom = top + glyph->size.y * scale;

				// スプライトと同じ並び（左下・左上・右下・右上）
				VertexData* vertex = &vertexData_[glyphIndex * kVertexPerGlyph];
				vertex[0].position = { left, bottom, 0.0f, 1.0f };
				vertex[0].texcoord = { glyph->uvMin.x, glyph->uvMax.y };
				vertex[1].position = { left, top, 0.0f, 1.0f };
				vertex[1].texcoord = { glyph->uvMin.x, glyph->uvMin.y };
				vertex[2].position = { right, bottom, 0.0f, 1.0f };
				vertex[2].texcoord = { glyph->uvMax.x, glyph->uvMax.y };
				vertex[3].position = { right, top, 0.0f, 1.0f };
				vertex[3].texcoord = { glyph->uvMax.x, glyph->uvMin.y };

				const uint32_t base = glyphIndex * kVertexPerGlyph;
				uint32_t* index = &indexData_[glyphIndex * kIndexPerGlyph];
				index[0] = base + 0;
				index[1] = base + 1;
				index[2] = base + 2;
				index[3] = base + 1;
				index[4] = base + 3;
				index[5] = base + 2;

				++glyphIndex;
			}

			pen += glyph->advance * scale;
			if (charIndex + 1 < line.size()) {
				pen += font_->GetKerning(line[charIndex], line[charIndex + 1]) * scale;
			}
		}
	}

	drawGlyphCount_ = glyphIndex;
}

void TextLabel::EnsureCapacity(uint32_t glyphCount) {
	if (hasBuffers_ && glyphCapacity_ >= glyphCount) return;

	// 1文字ずつ伸びるたびに作り直さないよう、多めに取る
	const uint32_t capacity = ((glyphCount + kCapacityGranularity - 1) / kCapacityGranularity) * kCapacityGranularity;

	ResourceManager* resourceManager = SpriteManager::GetInstance().GetDxManager()->GetResourceManager();

	if (hasBuffers_) {
		resourceManager->ReleaseBuffer(vertexHandle_);
		resourceManager->ReleaseBuffer(indexHandle_);
		hasBuffers_ = false;
	}

	const size_t vertexBytes = sizeof(VertexData) * kVertexPerGlyph * capacity;
	const size_t indexBytes = sizeof(uint32_t) * kIndexPerGlyph * capacity;

	vertexHandle_ = resourceManager->CreateUploadBuffer(vertexBytes, L"TextLabelVertex");
	vertexData_ = static_cast<VertexData*>(resourceManager->Map(vertexHandle_));
	assert(vertexData_);

	indexHandle_ = resourceManager->CreateUploadBuffer(indexBytes, L"TextLabelIndex");
	indexData_ = static_cast<uint32_t*>(resourceManager->Map(indexHandle_));
	assert(indexData_);

	vertexBufferView_.BufferLocation = resourceManager->GetResource(vertexHandle_)->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(vertexBytes);
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	indexBufferView_.BufferLocation = resourceManager->GetResource(indexHandle_)->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = static_cast<UINT>(indexBytes);
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	glyphCapacity_ = capacity;
	hasBuffers_ = true;
}

void TextLabel::CreateConstantBuffers() {
	ResourceManager* resourceManager = SpriteManager::GetInstance().GetDxManager()->GetResourceManager();

	// マテリアルはスプライトと同じ構造体を使う。ディゾルブなどは使わないので無効の値を入れておく
	auto createMaterial = [&](uint32_t& handle, SpriteMaterial*& data) {
		handle = resourceManager->CreateUploadBuffer(sizeof(SpriteMaterial), L"TextLabelMaterial");
		data = static_cast<SpriteMaterial*>(resourceManager->Map(handle));
		assert(data);
		data->color = { 1.0f, 1.0f, 1.0f, 1.0f };
		data->uvTransform = MakeIdentity4x4();
		data->dissolveThreshold = -1.0f;
		data->dissolveEdgeWidth = 0.05f;
		data->radialFill = -1.0f;
		data->dissolveEdgeColor = { 1.0f, 1.0f, 1.0f, 0.0f };
	};

	auto createTransform = [&](uint32_t& handle, TransformationMatrix*& data) {
		handle = resourceManager->CreateUploadBuffer(sizeof(TransformationMatrix), L"TextLabelTransform");
		data = static_cast<TransformationMatrix*>(resourceManager->Map(handle));
		assert(data);
		data->World = MakeIdentity4x4();
		data->WVP = MakeIdentity4x4();
	};

	createMaterial(materialHandle_, materialData_);
	createTransform(transformHandle_, transformData_);
	createMaterial(shadowMaterialHandle_, shadowMaterialData_);
	createTransform(shadowTransformHandle_, shadowTransformData_);
}

float TextLabel::MeasureLineWidth(const std::u32string& line) const {
	float width = 0.0f;

	for (size_t i = 0; i < line.size(); ++i) {
		const FontGlyph* glyph = font_->GetGlyph(line[i]);
		if (!glyph) continue;

		width += glyph->advance;
		if (i + 1 < line.size()) {
			width += font_->GetKerning(line[i], line[i + 1]);
		}
	}

	return width;
}
