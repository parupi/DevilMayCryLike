#include "OptionPanel.h"
#include "MenuNavigator.h"

#include <Audio/SoundManager.h>
#include <GameData/GameSettings.h>
#include <Graphics/Rendering/Sprite/SpriteManager.h>
#include <Input/Input.h>
#include <Utility/DeltaTime.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace {
	constexpr float kScreenWidth = 1280.0f;
	constexpr float kScreenHeight = 720.0f;
	constexpr float kCenterX = kScreenWidth * 0.5f;

	/// 音量を左右キー1回で動かす量
	constexpr float kVolumeStep = 0.05f;

	/// 行の表示名。OptionPanel::Row の並びと合わせること
	const char* kRowTexts[] = {
		"MASTER VOLUME",
		"BGM VOLUME",
		"SE VOLUME",
		"CAMERA SENSITIVITY",
		"INVERT CAMERA Y",
		"TUTORIAL",
	};

	/// 0.0～1.0 の値を 0～100 の表示用の数へ直す
	int32_t ToPercent(float value) {
		return static_cast<int32_t>(std::lround(value * 100.0f));
	}
}

void OptionPanel::Initialize()
{
	SpriteManager& sprites = SpriteManager::GetInstance();

	backdrop_ = sprites.CreateSprite(SpriteLayer::UI, "optBackdrop", "white.png");
	backdrop_->SetAnchorPoint({ 0.5f, 0.5f });
	backdrop_->SetPosition({ kCenterX, kScreenHeight * 0.5f });
	backdrop_->SetSize({ kScreenWidth, kScreenHeight });

	// 選択行の後ろに敷く加算グロー。タイトルの操作案内と同じ見せ方に揃えている
	highlight_ = sprites.CreateSprite(SpriteLayer::UI, "optHighlight", "circle.png");
	highlight_->SetAnchorPoint({ 0.5f, 0.5f });
	highlight_->SetSize({ 760.0f, 54.0f });
	highlight_->GetRenderState().blendMode = BlendMode::kAdd;

	cursor_ = sprites.CreateSprite(SpriteLayer::UI, "optCursor", "SelectArrow.png");
	cursor_->SetAnchorPoint({ 0.5f, 0.5f });
	cursor_->SetSize({ 26.0f, 26.0f });

	for (int32_t i = 0; i < kRowCount; ++i) {
		const Row row = static_cast<Row>(i);
		const std::string index = std::to_string(i);
		const float y = kRowStartY + kRowSpacing * i;

		RowSprites& sprite = rows_[i];

		sprite.label = sprites.CreateTextLabel(SpriteLayer::UI, "optLabel" + index);
		sprite.label->SetText(kRowTexts[i]);
		sprite.label->SetFontSize(kFontSize);
		sprite.label->SetAlign(TextAlignX::Left, TextAlignY::Middle);
		sprite.label->SetPosition({ kLabelX, y });
		sprite.label->SetShadow(true);

		sprite.value = sprites.CreateTextLabel(SpriteLayer::UI, "optValue" + index);
		sprite.value->SetFontSize(kFontSize);
		sprite.value->SetShadow(true);

		if (IsToggleRow(row)) {
			// ON/OFF はバーの左端に揃えて置く
			sprite.value->SetAlign(TextAlignX::Left, TextAlignY::Middle);
			sprite.value->SetPosition({ kValueX, y });
			continue;
		}

		sprite.value->SetAlign(TextAlignX::Right, TextAlignY::Middle);
		sprite.value->SetPosition({ kValueRightX, y });

		sprite.barBack = sprites.CreateSprite(SpriteLayer::UI, "optBarBack" + index, "white.png");
		sprite.barBack->SetAnchorPoint({ 0.0f, 0.5f });
		sprite.barBack->SetPosition({ kValueX, y });
		sprite.barBack->SetSize({ kBarWidth, kBarHeight });

		sprite.barFill = sprites.CreateSprite(SpriteLayer::UI, "optBarFill" + index, "white.png");
		sprite.barFill->SetAnchorPoint({ 0.0f, 0.5f });
		sprite.barFill->SetPosition({ kValueX, y });
		sprite.barFill->SetSize({ kBarWidth, kBarHeight });
	}

	title_ = sprites.CreateTextLabel(SpriteLayer::UI, "optTitle");
	title_->SetText("OPTION");
	title_->SetFontSize(48.0f);
	title_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	title_->SetPosition({ kCenterX, 128.0f });
	title_->SetShadow(true);

	hint_ = sprites.CreateTextLabel(SpriteLayer::UI, "optHint");
	hint_->SetFontSize(22.0f);
	hint_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	hint_->SetPosition({ kCenterX, 654.0f });
	hint_->SetShadow(true);

	// 開くまでは何も出さない
	Refresh();
}

void OptionPanel::Open()
{
	state_ = State::Appearing;
	selectedIndex_ = 0;
	pulseTimer_ = 0.0f;
}

void OptionPanel::Close()
{
	if (state_ == State::Hidden) return;

	// 触った値はここでファイルへ落とす。どちらも GlobalVariables 上の別グループなので個別に保存する
	SoundManager::GetInstance().SaveVolumes();
	GameSettings::GetInstance().Save();

	state_ = State::Closing;
}

void OptionPanel::Update(const MenuNavigator& navigator)
{
	const float deltaTime = DeltaTime::GetUnscaledDeltaTime();
	const float step = (kFadeTime > 0.0f) ? (deltaTime / kFadeTime) : 1.0f;

	switch (state_) {
	case State::Appearing:
		alpha_ = std::clamp(alpha_ + step, 0.0f, 1.0f);
		if (alpha_ >= 1.0f) state_ = State::Shown;
		break;
	case State::Closing:
		alpha_ = std::clamp(alpha_ - step, 0.0f, 1.0f);
		if (alpha_ <= 0.0f) state_ = State::Hidden;
		break;
	default:
		break;
	}

	// 操作を受けるのは出きってから
	if (state_ == State::Shown) {
		pulseTimer_ += deltaTime;

		if (navigator.IsUp()) {
			selectedIndex_ = (selectedIndex_ + kRowCount - 1) % kRowCount;
			SoundManager::GetInstance().PlaySE("SwordSlash", 0.25f);
		}
		if (navigator.IsDown()) {
			selectedIndex_ = (selectedIndex_ + 1) % kRowCount;
			SoundManager::GetInstance().PlaySE("SwordSlash", 0.25f);
		}
		if (navigator.IsLeft()) ChangeValue(-1);
		if (navigator.IsRight()) ChangeValue(+1);
	}

	Refresh();
}

void OptionPanel::ChangeValue(int32_t direction)
{
	SoundManager& sound = SoundManager::GetInstance();
	GameSettings& settings = GameSettings::GetInstance();

	switch (static_cast<Row>(selectedIndex_)) {
	case Row::MasterVolume:
		sound.SetMasterVolume(sound.GetMasterVolume() + kVolumeStep * direction);
		break;
	case Row::BgmVolume:
		sound.SetBGMVolume(sound.GetBGMVolume() + kVolumeStep * direction);
		break;
	case Row::SeVolume:
		sound.SetSEVolume(sound.GetSEVolume() + kVolumeStep * direction);
		// 変えた音量をその場で確かめられるように鳴らす
		sound.PlaySE("SwordHit", 1.0f);
		break;
	case Row::Sensitivity:
		settings.SetCameraSensitivity(settings.GetCameraSensitivity() + GameSettings::kSensitivityStep * direction);
		break;
	// ON/OFF は左で OFF・右で ON にする。
	// 押すたびに反転させると、押しっぱなしのリピートで往復してしまう
	case Row::InvertY:
		settings.SetInvertCameraY(direction > 0);
		break;
	case Row::Tutorial:
		settings.SetTutorialEnabled(direction > 0);
		break;
	default:
		break;
	}

	// SE の行は音量確認のSEを鳴らしているので、そちらに任せる
	if (static_cast<Row>(selectedIndex_) != Row::SeVolume) {
		sound.PlaySE("SwordSlash", 0.3f);
	}
}

void OptionPanel::Refresh()
{
	const bool visible = (state_ != State::Hidden);
	const bool usePad = Input::GetInstance().IsConnected();
	const SoundManager& sound = SoundManager::GetInstance();
	const GameSettings& settings = GameSettings::GetInstance();

	backdrop_->SetColor({ 0.0f, 0.0f, 0.0f, kBackdropAlpha * alpha_ });
	backdrop_->GetRenderState().isVisible = visible;
	backdrop_->Update();

	title_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ });
	title_->GetRenderState().isVisible = visible;
	title_->Update();

	// 選択行の目印。ゆっくり明滅させて、どこを触っているか分かるようにする
	const float pulse = 0.5f + 0.5f * std::cos(pulseTimer_ * 3.0f);
	const float selectedY = kRowStartY + kRowSpacing * selectedIndex_;

	highlight_->SetPosition({ 665.0f, selectedY });
	highlight_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ * (0.05f + 0.09f * pulse) });
	highlight_->GetRenderState().isVisible = visible;
	highlight_->Update();

	cursor_->SetPosition({ kCursorX - 4.0f * pulse, selectedY });
	cursor_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ });
	cursor_->GetRenderState().isVisible = visible;
	cursor_->Update();

	for (int32_t i = 0; i < kRowCount; ++i) {
		RowSprites& row = rows_[i];
		const bool isSelected = (i == selectedIndex_);
		const float brightness = isSelected ? 1.0f : kDimBrightness;

		row.label->SetColor({ brightness, brightness, brightness, alpha_ });
		row.label->GetRenderState().isVisible = visible;
		row.label->Update();

		switch (static_cast<Row>(i)) {
		case Row::MasterVolume:
			RefreshNumberRow(row, sound.GetMasterVolume(), ToPercent(sound.GetMasterVolume()), brightness);
			break;
		case Row::BgmVolume:
			RefreshNumberRow(row, sound.GetBGMVolume(), ToPercent(sound.GetBGMVolume()), brightness);
			break;
		case Row::SeVolume:
			RefreshNumberRow(row, sound.GetSEVolume(), ToPercent(sound.GetSEVolume()), brightness);
			break;
		case Row::Sensitivity: {
			// 感度は 0.2～2.0 なので、バーの伸びは範囲内での位置で出す
			const float sensitivity = settings.GetCameraSensitivity();
			const float ratio = (sensitivity - GameSettings::kMinSensitivity)
				/ (GameSettings::kMaxSensitivity - GameSettings::kMinSensitivity);
			RefreshNumberRow(row, ratio, ToPercent(sensitivity), brightness);
			break;
		}
		case Row::InvertY:
			RefreshToggleRow(row, settings.IsInvertCameraY(), brightness);
			break;
		case Row::Tutorial:
			RefreshToggleRow(row, settings.IsTutorialEnabled(), brightness);
			break;
		default:
			break;
		}
	}

	// 矢印は記号のまま出す（フォントが Noto Sans JP なので↑↓←→も持っている）
	hint_->SetText(usePad
		? "↑↓ : SELECT     ←→ : CHANGE     B : BACK"
		: "W S : SELECT     A D : CHANGE     ESC : BACK");
	hint_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ * 0.8f });
	hint_->GetRenderState().isVisible = visible;
	hint_->Update();
}

void OptionPanel::RefreshNumberRow(RowSprites& row, float ratio, int32_t number, float brightness)
{
	const bool visible = (state_ != State::Hidden);
	const float clampedRatio = std::clamp(ratio, 0.0f, 1.0f);

	row.barBack->SetColor({ brightness, brightness, brightness, alpha_ * 0.25f });
	row.barBack->GetRenderState().isVisible = visible;
	row.barBack->Update();

	// 伸びるのは幅だけ。左端で固定したいのでアンカーは生成時に左へ寄せてある
	row.barFill->SetSize({ kBarWidth * clampedRatio, kBarHeight });
	row.barFill->SetColor({ 0.55f * brightness, 0.78f * brightness, 1.0f * brightness, alpha_ });
	row.barFill->GetRenderState().isVisible = visible && clampedRatio > 0.0f;
	row.barFill->Update();

	row.value->SetText(std::to_string(number));
	row.value->SetColor({ brightness, brightness, brightness, alpha_ });
	row.value->GetRenderState().isVisible = visible;
	row.value->Update();
}

void OptionPanel::RefreshToggleRow(RowSprites& row, bool value, float brightness)
{
	const bool visible = (state_ != State::Hidden);

	row.value->SetText(value ? "ON" : "OFF");
	row.value->SetColor({ brightness, brightness, brightness, alpha_ });
	row.value->GetRenderState().isVisible = visible;
	row.value->Update();
}
