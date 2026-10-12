#include "ActionGuideUI.h"
#include "ActionGuideSettings.h"
#include "InputIconProvider.h"

#include "Graphics/Rendering/Sprite/Sprite.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include "Graphics/Resource/TextureManager.h"
#include "Graphics/Text/TextLabel.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace {
	constexpr const char* kCircleTexture = "UI/ActionGuide/Circle";
	constexpr const char* kCapTexture = "UI/ActionGuide/Cap";
	constexpr const char* kStripTexture = "UI/ActionGuide/Strip";

	constexpr uint32_t kCircleSize = 64;
	constexpr uint32_t kCapWidth = 20;
	constexpr uint32_t kCapHeight = 64;
	constexpr uint32_t kStripWidth = 256;
	constexpr uint32_t kStripHeight = 32;

	/// <summary>横長アイコンの角の丸みの大きさ（高さに対する割合）。左端の絵の幅もこれ</summary>
	constexpr float kCapRatio = static_cast<float>(kCapWidth) / static_cast<float>(kCapHeight);
	/// <summary>帯がアイコンの左へはみ出す量（組み合わせの文字の分）[px]</summary>
	constexpr float kStripLeftPad = 130.0f;

	// 押し方の文字の色（名前と見分けるための暖色）
	constexpr Vector4 kGestureColor = { 1.0f, 0.82f, 0.48f, 1.0f };
	constexpr Vector4 kPrefixColor = { 0.82f, 0.82f, 0.82f, 1.0f };

	float SmoothStep01(float edge0, float edge1, float x) {
		const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	void WritePixel(std::vector<uint8_t>& pixels, uint32_t width, uint32_t x, uint32_t y, float alpha) {
		const size_t index = (static_cast<size_t>(y) * width + x) * 4;
		pixels[index + 0] = 255;
		pixels[index + 1] = 255;
		pixels[index + 2] = 255;
		pixels[index + 3] = static_cast<uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f + 0.5f);
	}

	// 縁をなめらかにした白い丸。色は Sprite の SetColor で付ける
	void EnsureCircleTexture() {
		if (TextureManager::GetInstance().TryGetMetaData(kCircleTexture) != nullptr) return;

		std::vector<uint8_t> pixels(static_cast<size_t>(kCircleSize) * kCircleSize * 4);
		const float center = kCircleSize * 0.5f;
		const float radius = center - 1.0f;
		for (uint32_t y = 0; y < kCircleSize; ++y) {
			for (uint32_t x = 0; x < kCircleSize; ++x) {
				const float dx = x + 0.5f - center;
				const float dy = y + 0.5f - center;
				const float distance = std::sqrt(dx * dx + dy * dy);
				WritePixel(pixels, kCircleSize, x, y, radius - distance + 0.5f);
			}
		}
		TextureManager::GetInstance().LoadTextureFromMemory(kCircleTexture, pixels.data(), kCircleSize, kCircleSize);
	}

	// 横長アイコンの左端：左上と左下が丸い。右端は Sprite の左右反転で使い回す
	void EnsureCapTexture() {
		if (TextureManager::GetInstance().TryGetMetaData(kCapTexture) != nullptr) return;

		std::vector<uint8_t> pixels(static_cast<size_t>(kCapWidth) * kCapHeight * 4);
		const float radius = static_cast<float>(kCapWidth);
		for (uint32_t y = 0; y < kCapHeight; ++y) {
			for (uint32_t x = 0; x < kCapWidth; ++x) {
				const float px = x + 0.5f;
				const float py = y + 0.5f;
				// 角の円の中心（上の角と下の角）。その間はまっすぐな縁
				const float cy = std::clamp(py, radius, kCapHeight - radius);
				const float dx = px - radius;
				const float dy = py - cy;
				const float distance = std::sqrt(dx * dx + dy * dy);
				const float alpha = (px >= radius) ? 1.0f : (radius - 0.5f - distance + 0.5f);
				WritePixel(pixels, kCapWidth, x, y, alpha);
			}
		}
		TextureManager::GetInstance().LoadTextureFromMemory(kCapTexture, pixels.data(), kCapWidth, kCapHeight);
	}

	// 行の後ろの帯：左右の端を透かした横長の矩形
	void EnsureStripTexture() {
		if (TextureManager::GetInstance().TryGetMetaData(kStripTexture) != nullptr) return;

		std::vector<uint8_t> pixels(static_cast<size_t>(kStripWidth) * kStripHeight * 4);
		for (uint32_t y = 0; y < kStripHeight; ++y) {
			for (uint32_t x = 0; x < kStripWidth; ++x) {
				const float px = x + 0.5f;
				const float py = y + 0.5f;
				// 左は長めに透かす（組み合わせの文字が無い行で帯の端が目立たないように）
				const float horizontal = SmoothStep01(0.0f, 70.0f, px) * SmoothStep01(0.0f, 24.0f, kStripWidth - px);
				const float vertical = SmoothStep01(0.0f, 6.0f, py) * SmoothStep01(0.0f, 6.0f, kStripHeight - py);
				WritePixel(pixels, kStripWidth, x, y, horizontal * vertical);
			}
		}
		TextureManager::GetInstance().LoadTextureFromMemory(kStripTexture, pixels.data(), kStripWidth, kStripHeight);
	}

	void SetVisible(Sprite* sprite, bool visible) {
		sprite->GetRenderState().isVisible = visible;
		sprite->Update();
	}
	void SetVisible(TextLabel* label, bool visible) {
		label->GetRenderState().isVisible = visible;
		label->Update();
	}

	Vector4 WithAlpha(const Vector4& color, float alpha) {
		return { color.x, color.y, color.z, color.w * alpha };
	}
}

void ActionGuideUI::Initialize()
{
	// CreateSprite は登録済みの名前ならファイルを読みに行かないので、先にテクスチャを作る
	EnsureCircleTexture();
	EnsureCapTexture();
	EnsureStripTexture();

	SpriteManager& sprites = SpriteManager::GetInstance();
	for (size_t i = 0; i < rows_.size(); ++i) {
		Row& row = rows_[i];
		const std::string id = "actionGuide" + std::to_string(i);

		// 奥から順に作る（同じレイヤーでは後から作ったものが手前に出る）
		row.strip = sprites.CreateSprite(SpriteLayer::UI, id + "Strip", kStripTexture);
		row.strip->SetAnchorPoint({ 0.0f, 0.5f });

		row.dot = sprites.CreateSprite(SpriteLayer::UI, id + "Dot", kCircleTexture);
		row.dot->SetAnchorPoint({ 0.5f, 0.5f });

		row.circle = sprites.CreateSprite(SpriteLayer::UI, id + "Circle", kCircleTexture);
		row.circle->SetAnchorPoint({ 0.5f, 0.5f });

		row.capLeft = sprites.CreateSprite(SpriteLayer::UI, id + "CapL", kCapTexture);
		row.capLeft->SetAnchorPoint({ 0.0f, 0.5f });
		row.middle = sprites.CreateSprite(SpriteLayer::UI, id + "Mid", "white.png");
		row.middle->SetAnchorPoint({ 0.0f, 0.5f });
		// 反転するとアンカーの反対側（左）へ伸びるので、ApplyRow では右端の位置に置く
		row.capRight = sprites.CreateSprite(SpriteLayer::UI, id + "CapR", kCapTexture);
		row.capRight->SetAnchorPoint({ 0.0f, 0.5f });
		row.capRight->SetIsFlipX(true);
		// 端の丸みの絵は、拡大すると内側の端で反対側の（半透明の）列と混ざって継ぎ目に縦線が出る。
		// 最後の1列を使わないようにして、中の白い矩形とつながる側を不透明のままにする
		for (Sprite* cap : { row.capLeft, row.capRight }) {
			cap->SetTextureSize({ static_cast<float>(kCapWidth - 1), static_cast<float>(kCapHeight) });
		}

		row.iconText = sprites.CreateTextLabel(SpriteLayer::UI, id + "IconText");
		row.iconText->SetAlign(TextAlignX::Center, TextAlignY::Middle);

		row.prefix = sprites.CreateTextLabel(SpriteLayer::UI, id + "Prefix");
		row.prefix->SetAlign(TextAlignX::Right, TextAlignY::Middle);
		row.prefix->SetShadow(true);

		row.gesture = sprites.CreateTextLabel(SpriteLayer::UI, id + "Gesture");
		row.gesture->SetAlign(TextAlignX::Left, TextAlignY::Middle);
		row.gesture->SetShadow(true);

		row.name = sprites.CreateTextLabel(SpriteLayer::UI, id + "Name");
		row.name->SetAlign(TextAlignX::Left, TextAlignY::Middle);
		row.name->SetShadow(true);

		HideRow(row);
	}
}

void ActionGuideUI::HideRow(Row& row)
{
	for (Sprite* sprite : { row.strip, row.dot, row.circle, row.capLeft, row.middle, row.capRight }) {
		SetVisible(sprite, false);
	}
	for (TextLabel* label : { row.iconText, row.prefix, row.gesture, row.name }) {
		SetVisible(label, false);
	}
}

void ActionGuideUI::Update(const ActionGuideViewModel& viewModel, const ActionGuideSettings& settings,
	bool allowed, float raise, float deltaTime)
{
	allowed_ = allowed;
	const float fadeSpeed = (settings.fadeTime > 0.0f) ? (1.0f / settings.fadeTime) : 1000.0f;

	// ガイド全体の出し入れ。ポーズなどの間は暗幕の下に半端に残らないよう即座に消す
	if (!allowed_) {
		guideAlpha_ = 0.0f;
	} else {
		const float target = viewModel.visible ? 1.0f : 0.0f;
		guideAlpha_ = (guideAlpha_ < target)
			? (std::min)(target, guideAlpha_ + fadeSpeed * deltaTime)
			: (std::max)(target, guideAlpha_ - fadeSpeed * deltaTime);
	}

	const bool usePad = InputIconProvider::UsePad();
	const int32_t slotCount = std::clamp(settings.maxEntries, 1, static_cast<int32_t>(rows_.size()));

	for (int32_t i = 0; i < static_cast<int32_t>(rows_.size()); ++i) {
		Row& row = rows_[i];

		const bool hasEntry = viewModel.visible && i < static_cast<int32_t>(viewModel.entries.size()) && i < slotCount;
		if (hasEntry) {
			const ActionGuideEntry& entry = viewModel.entries[i];
			// 別の行動に入れ替わったら、フェードの始まりから出し直す（仕様書 §6-7）。
			// 0 から始めないのは、1〜2フレームしかない受付窓で表示が遅れて情報が隠れないようにするため
			const bool switched = !row.hasEntry || row.entry.id != entry.id || !(row.entry.binding == entry.binding);
			if (switched) {
				row.presence = (std::min)(row.presence, settings.fadeStartAlpha);
			}
			row.entry = entry;
			row.hasEntry = true;
			row.presence = (std::min)(1.0f, row.presence + fadeSpeed * deltaTime);
		} else {
			row.hasEntry = false;
			row.presence = (std::max)(0.0f, row.presence - fadeSpeed * deltaTime);
		}

		if (row.presence <= 0.0f || guideAlpha_ <= 0.0f || i >= slotCount) {
			HideRow(row);
			continue;
		}
		ApplyRow(row, i, settings, raise, usePad);
	}
}

void ActionGuideUI::ApplyRow(Row& row, int32_t index, const ActionGuideSettings& settings, float raise, bool usePad)
{
	const ActionGuideEntry& entry = row.entry;
	const int32_t slotCount = std::clamp(settings.maxEntries, 1, static_cast<int32_t>(rows_.size()));

	// 行の位置は固定の枠。上から優先度順に詰め、件数が減っても行が上下に動かないようにする
	const float y = settings.bottomY - raise - settings.rowSpacing * static_cast<float>(slotCount - 1 - index);
	// 入れ替わった直後は右から少し滑り込ませる
	const float slide = settings.slideOffset * (1.0f - row.presence);
	const float iconX = settings.iconX + slide;

	const float entryAlpha = (entry.availability == ActionAvailability::Preview) ? settings.previewAlpha : settings.normalAlpha;
	const float alpha = row.presence * entryAlpha * guideAlpha_;

	// ── 帯 ──
	row.strip->SetPosition({ iconX - kStripLeftPad, y });
	row.strip->SetSize({ settings.backdropWidth + kStripLeftPad, settings.rowSpacing - 4.0f });
	row.strip->SetColor({ 0.0f, 0.0f, 0.0f, settings.backdropAlpha * row.presence * guideAlpha_ });
	SetVisible(row.strip, true);

	// ── アイコン ──
	const InputIcon icon = InputIconProvider::Resolve(entry.binding.action, usePad);
	const float iconHeight = settings.iconSize;
	const float iconWidth = (icon.shape == InputIconShape::Circle) ? iconHeight : iconHeight * icon.widthScale;
	const float iconLeft = iconX - iconWidth * 0.5f;

	if (icon.shape == InputIconShape::Circle) {
		row.circle->SetPosition({ iconX, y });
		row.circle->SetSize({ iconHeight, iconHeight });
		row.circle->SetColor(WithAlpha(icon.fillColor, alpha));
		SetVisible(row.circle, true);
		SetVisible(row.capLeft, false);
		SetVisible(row.middle, false);
		SetVisible(row.capRight, false);
	} else {
		const float capWidth = iconHeight * kCapRatio;
		const float middleWidth = (std::max)(0.0f, iconWidth - capWidth * 2.0f);
		const Vector4 color = WithAlpha(icon.fillColor, alpha);

		row.capLeft->SetPosition({ iconLeft, y });
		row.capLeft->SetSize({ capWidth, iconHeight });
		row.capLeft->SetColor(color);
		row.middle->SetPosition({ iconLeft + capWidth, y });
		row.middle->SetSize({ middleWidth, iconHeight });
		row.middle->SetColor(color);
		// 左右反転したスプライトはアンカーから左へ伸びるので、右端の位置に置く
		row.capRight->SetPosition({ iconLeft + capWidth * 2.0f + middleWidth, y });
		row.capRight->SetSize({ capWidth, iconHeight });
		row.capRight->SetColor(color);

		SetVisible(row.circle, false);
		SetVisible(row.capLeft, true);
		SetVisible(row.middle, true);
		SetVisible(row.capRight, true);
	}

	row.iconText->SetText(icon.text);
	row.iconText->SetFontSize(iconHeight * (icon.text.size() > 2 ? 0.42f : 0.62f));
	row.iconText->SetPosition({ iconX, y });
	row.iconText->SetColor(WithAlpha(icon.textColor, alpha));
	SetVisible(row.iconText, true);

	// ── 組み合わせ（アイコンの左） ──
	const std::string prefix = InputIconProvider::PrefixText(entry.binding, usePad);
	row.prefix->SetText(prefix);
	row.prefix->SetFontSize(settings.fontSize * 0.85f);
	row.prefix->SetPosition({ iconLeft - settings.textGap * 0.5f, y });
	row.prefix->SetColor(WithAlpha(kPrefixColor, alpha));
	SetVisible(row.prefix, !prefix.empty());
	const float prefixWidth = prefix.empty() ? 0.0f : row.prefix->GetSize().x;

	// ── 先行入力済みの印（組み合わせの左） ──
	if (entry.availability == ActionAvailability::Buffered && entry.queued) {
		const float dotX = iconLeft - settings.textGap * 0.5f - prefixWidth - settings.queuedMarkSize;
		row.dot->SetPosition({ dotX, y });
		row.dot->SetSize({ settings.queuedMarkSize, settings.queuedMarkSize });
		row.dot->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
		SetVisible(row.dot, true);
	} else {
		SetVisible(row.dot, false);
	}

	// ── 押し方と名前（アイコンの右） ──
	float textX = iconX + iconWidth * 0.5f + settings.textGap;
	const std::string gesture = InputIconProvider::GestureText(entry.binding, settings);
	row.gesture->SetText(gesture);
	row.gesture->SetFontSize(settings.gestureFontSize);
	row.gesture->SetPosition({ textX, y });
	row.gesture->SetColor(WithAlpha(kGestureColor, alpha));
	SetVisible(row.gesture, !gesture.empty());
	if (!gesture.empty()) {
		textX += row.gesture->GetSize().x + settings.textGap * 0.6f;
	}

	row.name->SetText(entry.displayName);
	row.name->SetFontSize(settings.fontSize);
	row.name->SetPosition({ textX, y });
	row.name->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
	SetVisible(row.name, true);
}
