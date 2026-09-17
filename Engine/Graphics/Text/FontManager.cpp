#include "FontManager.h"

FontManager& FontManager::GetInstance() {
	static FontManager instance;
	return instance;
}

Font* FontManager::LoadFont(const std::string& name, const std::string& ttfPath, float pixelHeight, uint32_t atlasSize) {
	if (auto it = fonts_.find(name); it != fonts_.end()) {
		return it->second.get();
	}

	auto font = std::make_unique<Font>();
	if (!font->Initialize(name, ttfPath, pixelHeight, atlasSize)) {
		return nullptr;
	}

	Font* result = font.get();
	fonts_[name] = std::move(font);

	// 最初に読んだものを既定にしておく（明示的に変えたければ SetDefaultFont）
	if (!defaultFont_) {
		defaultFont_ = result;
	}
	return result;
}

Font* FontManager::Find(const std::string& name) {
	auto it = fonts_.find(name);
	return (it != fonts_.end()) ? it->second.get() : nullptr;
}

void FontManager::SetDefaultFont(const std::string& name) {
	if (Font* font = Find(name)) {
		defaultFont_ = font;
	}
}

void FontManager::FlushAtlases() {
	for (auto& [name, font] : fonts_) {
		font->FlushAtlas();
	}
}

void FontManager::Finalize() {
	fonts_.clear();
	defaultFont_ = nullptr;
}
