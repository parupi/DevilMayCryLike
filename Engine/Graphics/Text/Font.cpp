#include "Font.h"
#include "Graphics/Resource/TextureManager.h"
#include "Utility/Logger.h"

#pragma warning(push, 0)
#include <imstb_truetype.h>
#pragma warning(pop)

#include <algorithm>
#include <fstream>

Font::Font() = default;

Font::~Font() = default;

bool Font::Initialize(const std::string& name, const std::string& ttfPath, float pixelHeight, uint32_t atlasSize) {
	name_ = name;
	pixelHeight_ = pixelHeight;
	atlasSize_ = atlasSize;

	std::ifstream file(ttfPath, std::ios::binary | std::ios::ate);
	if (!file) {
		Logger::Log("[Font] フォントを開けません: " + ttfPath + "\n");
		return false;
	}

	const std::streamoff size = file.tellg();
	file.seekg(0, std::ios::beg);
	ttfData_.resize(static_cast<size_t>(size));
	file.read(reinterpret_cast<char*>(ttfData_.data()), size);

	info_ = std::make_unique<stbtt_fontinfo>();
	const int offset = stbtt_GetFontOffsetForIndex(ttfData_.data(), 0);
	if (offset < 0 || !stbtt_InitFont(info_.get(), ttfData_.data(), offset)) {
		Logger::Log("[Font] フォントを解釈できません: " + ttfPath + "\n");
		info_.reset();
		ttfData_.clear();
		return false;
	}

	scale_ = stbtt_ScaleForPixelHeight(info_.get(), pixelHeight_);

	int ascent = 0;
	int descent = 0;
	int lineGap = 0;
	stbtt_GetFontVMetrics(info_.get(), &ascent, &descent, &lineGap);
	ascent_ = ascent * scale_;
	descent_ = descent * scale_;
	lineHeight_ = (ascent - descent + lineGap) * scale_;

	// 全面を透明で始める。色は白のまま置いて、字形の濃さはアルファだけで表す
	atlasPixels_.assign(static_cast<size_t>(atlasSize_) * atlasSize_ * 4, 0);
	for (size_t i = 0; i < atlasPixels_.size(); i += 4) {
		atlasPixels_[i + 0] = 255;
		atlasPixels_[i + 1] = 255;
		atlasPixels_[i + 2] = 255;
	}

	// 他のテクスチャと名前がぶつからないようにしておく
	atlasTextureName_ = "__FONT_" + name_ + "__";
	TextureManager::GetInstance().LoadTextureFromMemory(atlasTextureName_, atlasPixels_.data(), atlasSize_, atlasSize_);

	Logger::Log("[Font] 読み込み完了: " + name_ + " (" + std::to_string(static_cast<int>(pixelHeight_)) + "px)\n");
	return true;
}

const FontGlyph* Font::GetGlyph(char32_t codePoint) {
	if (!info_) return nullptr;

	// unordered_map は再ハッシュしても要素のアドレスが変わらないので、そのまま返してよい
	auto it = glyphs_.find(codePoint);
	if (it != glyphs_.end()) {
		return &it->second;
	}

	return Rasterize(codePoint);
}

float Font::GetKerning(char32_t left, char32_t right) const {
	if (!info_) return 0.0f;

	return stbtt_GetCodepointKernAdvance(info_.get(), static_cast<int>(left), static_cast<int>(right)) * scale_;
}

void Font::FlushAtlas() {
	if (!atlasDirty_) return;

	TextureManager::GetInstance().UpdateTextureFromMemory(atlasTextureName_, atlasPixels_.data(), atlasSize_, atlasSize_);
	atlasDirty_ = false;
}

const FontGlyph* Font::Rasterize(char32_t codePoint) {
	FontGlyph glyph{};

	int advanceWidth = 0;
	int leftSideBearing = 0;
	stbtt_GetCodepointHMetrics(info_.get(), static_cast<int>(codePoint), &advanceWidth, &leftSideBearing);
	glyph.advance = advanceWidth * scale_;

	int width = 0;
	int height = 0;
	int xOffset = 0;
	int yOffset = 0;
	unsigned char* bitmap = stbtt_GetCodepointBitmap(
		info_.get(), 0.0f, scale_, static_cast<int>(codePoint), &width, &height, &xOffset, &yOffset);

	// 空白のように字形を持たない文字。送り幅だけ覚えておく
	if (!bitmap || width <= 0 || height <= 0) {
		if (bitmap) stbtt_FreeBitmap(bitmap, nullptr);
		return &(glyphs_[codePoint] = glyph);
	}

	uint32_t x = 0;
	uint32_t y = 0;
	if (!AllocateRect(static_cast<uint32_t>(width), static_cast<uint32_t>(height), x, y)) {
		stbtt_FreeBitmap(bitmap, nullptr);
		if (!atlasFull_) {
			atlasFull_ = true;
			Logger::Log("[Font] アトラスが満杯です: " + name_ + "。これ以降の新しい文字は出ません\n");
		}
		return nullptr;
	}

	// 濃さをアルファへ書き込む
	for (int row = 0; row < height; ++row) {
		const size_t dstRow = (static_cast<size_t>(y) + row) * atlasSize_;
		for (int column = 0; column < width; ++column) {
			const size_t dst = (dstRow + x + column) * 4;
			atlasPixels_[dst + 3] = bitmap[static_cast<size_t>(row) * width + column];
		}
	}
	stbtt_FreeBitmap(bitmap, nullptr);

	const float atlasSizeF = static_cast<float>(atlasSize_);
	glyph.uvMin = { x / atlasSizeF, y / atlasSizeF };
	glyph.uvMax = { (x + width) / atlasSizeF, (y + height) / atlasSizeF };
	glyph.size = { static_cast<float>(width), static_cast<float>(height) };
	glyph.bearing = { static_cast<float>(xOffset), static_cast<float>(yOffset) };

	atlasDirty_ = true;
	return &(glyphs_[codePoint] = glyph);
}

bool Font::AllocateRect(uint32_t width, uint32_t height, uint32_t& outX, uint32_t& outY) {
	if (width + kGlyphPadding > atlasSize_) return false;

	// 今の段に入らなければ、一番高い字形の下へ棚を1段下げる
	if (shelfX_ + width + kGlyphPadding > atlasSize_) {
		shelfY_ += shelfHeight_ + kGlyphPadding;
		shelfX_ = 0;
		shelfHeight_ = 0;
	}

	if (shelfY_ + height + kGlyphPadding > atlasSize_) return false;

	outX = shelfX_;
	outY = shelfY_;

	shelfX_ += width + kGlyphPadding;
	shelfHeight_ = (std::max)(shelfHeight_, height);
	return true;
}
