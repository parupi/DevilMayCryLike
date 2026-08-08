#include "MenuItemList.h"
#include "MenuNavigator.h"

#include <Audio/SoundManager.h>
#include <Graphics/Rendering/Sprite/SpriteManager.h>
#include <Graphics/Resource/TextureManager.h>

#include <algorithm>
#include <cmath>

void MenuItemList::Initialize(const std::string& idPrefix, SpriteLayer layer,
	const std::vector<std::string>& items,
	const Vector2& firstItemCenter, float spacing, float fontSize) {
	firstItemCenter_ = firstItemCenter;
	spacing_ = spacing;

	TextureManager::GetInstance().LoadTexture("circle.png");
	TextureManager::GetInstance().LoadTexture("SelectArrow.png");

	SpriteManager& sprites = SpriteManager::GetInstance();

	// 選択中の項目の後ろに敷く加算グロー
	highlight_ = sprites.CreateSprite(layer, idPrefix + "Highlight", "circle.png");
	highlight_->SetAnchorPoint({ 0.5f, 0.5f });
	highlight_->SetSize({ 440.0f, spacing });
	highlight_->GetRenderState().blendMode = BlendMode::kAdd;

	for (int32_t i = 0; i < 2; ++i) {
		arrows_[i] = sprites.CreateSprite(layer, idPrefix + "Arrow" + std::to_string(i), "SelectArrow.png");
		arrows_[i]->SetAnchorPoint({ 0.5f, 0.5f });
		// 右側は x を負にして左右反転させ、内向きの矢印にする
		arrows_[i]->SetSize({ (i == 0) ? 28.0f : -28.0f, 28.0f });
	}

	items_.reserve(items.size());
	for (size_t i = 0; i < items.size(); ++i) {
		TextLabel* label = sprites.CreateTextLabel(layer, idPrefix + "Item" + std::to_string(i));
		label->SetText(items[i]);
		label->SetFontSize(fontSize);
		label->SetAlign(TextAlignX::Center, TextAlignY::Middle);
		label->SetPosition({ firstItemCenter_.x, firstItemCenter_.y + spacing_ * i });
		label->SetShadow(true);
		items_.push_back(label);
	}
}

bool MenuItemList::UpdateSelection(const MenuNavigator& navigator) {
	if (items_.empty()) return false;

	const int32_t count = GetItemCount();
	bool moved = false;

	if (navigator.IsUp()) {
		selectedIndex_ = (selectedIndex_ + count - 1) % count;
		moved = true;
	}
	if (navigator.IsDown()) {
		selectedIndex_ = (selectedIndex_ + 1) % count;
		moved = true;
	}

	if (moved) {
		SoundManager::GetInstance().PlaySE("SwordSlash", 0.25f);
	}
	return moved;
}

void MenuItemList::Refresh(float alpha, float deltaTime) {
	if (items_.empty()) return;

	const bool visible = alpha > 0.0f;

	pulseTimer_ += deltaTime;
	// 0.0～1.0 を往復する明滅の係数
	const float pulse = 0.5f + 0.5f * std::cos(pulseTimer_ * 3.0f);
	const float selectedY = firstItemCenter_.y + spacing_ * selectedIndex_;

	highlight_->SetPosition({ firstItemCenter_.x, selectedY });
	highlight_->SetColor({ 1.0f, 1.0f, 1.0f, alpha * (0.05f + 0.10f * pulse) });
	highlight_->GetRenderState().isVisible = visible;
	highlight_->Update();

	for (int32_t i = 0; i < GetItemCount(); ++i) {
		const float brightness = (i == selectedIndex_) ? 1.0f : kDimBrightness;
		items_[i]->SetColor({ brightness, brightness, brightness, alpha });
		items_[i]->GetRenderState().isVisible = visible;
		items_[i]->Update();
	}

	// 矢印は選択中の項目の幅に合わせて置く。項目ごとに文字数が違うので、
	// 固定位置にすると短い項目のときだけ間延びして見える
	const float halfWidth = items_[selectedIndex_]->GetSize().x * 0.5f;
	for (int32_t i = 0; i < 2; ++i) {
		// 明滅に合わせて外へ広げ、選択中の項目を押し出しているように見せる
		const float offset = halfWidth + kArrowMargin + 6.0f * pulse;
		arrows_[i]->SetPosition({ firstItemCenter_.x + ((i == 0) ? -offset : offset), selectedY });
		arrows_[i]->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
		arrows_[i]->GetRenderState().isVisible = visible;
		arrows_[i]->Update();
	}
}

void MenuItemList::SetSelectedIndex(int32_t index) {
	if (items_.empty()) return;
	selectedIndex_ = std::clamp(index, 0, GetItemCount() - 1);
}

void MenuItemList::SetItemText(int32_t index, const std::string& text) {
	if (index < 0 || index >= GetItemCount()) return;
	items_[index]->SetText(text);
}
