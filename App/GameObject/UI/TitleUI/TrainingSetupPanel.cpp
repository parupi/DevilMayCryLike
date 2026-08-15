#include "TrainingSetupPanel.h"
#include "GameObject/UI/Common/MenuNavigator.h"

#include <Audio/SoundManager.h>
#include <GameData/EnemyCatalog.h>
#include <Graphics/Rendering/Sprite/SpriteManager.h>
#include <Input/Input.h>
#include <Utility/DeltaTime.h>

#include <algorithm>

namespace {
	constexpr float kScreenWidth = 1280.0f;
	constexpr float kScreenHeight = 720.0f;
	constexpr float kCenterX = kScreenWidth * 0.5f;

	const std::string kEmptyClassName;
}

void TrainingSetupPanel::Initialize()
{
	SpriteManager& sprites = SpriteManager::GetInstance();

	backdrop_ = sprites.CreateSprite(SpriteLayer::UI, "trainBackdrop", "white.png");
	backdrop_->SetAnchorPoint({ 0.5f, 0.5f });
	backdrop_->SetPosition({ kCenterX, kScreenHeight * 0.5f });
	backdrop_->SetSize({ kScreenWidth, kScreenHeight });

	title_ = sprites.CreateTextLabel(SpriteLayer::UI, "trainTitle");
	title_->SetText("TRAINING");
	title_->SetFontSize(48.0f);
	title_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	title_->SetPosition({ kCenterX, 128.0f });
	title_->SetShadow(true);

	// 並べる敵は EnemyCatalog 任せ。敵を増やしてもここは触らない
	std::vector<std::string> names;
	for (const auto& entry : EnemyCatalog::GetEntries()) {
		names.push_back(entry.displayName);
	}
	itemList_.Initialize("trainMenu", SpriteLayer::UI, names,
		{ kCenterX, kItemStartY }, kItemSpacing, kItemFontSize);

	description_ = sprites.CreateTextLabel(SpriteLayer::UI, "trainDescription");
	description_->SetFontSize(24.0f);
	description_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	description_->SetPosition({ kCenterX, kDescriptionY });
	description_->SetShadow(true);

	hint_ = sprites.CreateTextLabel(SpriteLayer::UI, "trainHint");
	hint_->SetFontSize(22.0f);
	hint_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	hint_->SetPosition({ kCenterX, 654.0f });
	hint_->SetShadow(true);

	// 開くまでは何も出さない
	Refresh(0.0f);
}

void TrainingSetupPanel::Open()
{
	state_ = State::Appearing;
	itemList_.SetSelectedIndex(0);
}

void TrainingSetupPanel::Close()
{
	if (state_ == State::Hidden) return;
	state_ = State::Closing;
}

TrainingSetupPanel::Result TrainingSetupPanel::Update(const MenuNavigator& navigator)
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

	Result result = Result::None;

	// 操作を受けるのは出きってから。
	// 開いた瞬間の入力を拾って即決定してしまうのを防ぐ
	if (state_ == State::Shown) {
		itemList_.UpdateSelection(navigator);

		if (navigator.IsDecide()) {
			SoundManager::GetInstance().PlaySE("SwordHit", 0.6f);
			result = Result::Decided;
		} else if (navigator.IsCancel()) {
			result = Result::Canceled;
		}
	}

	Refresh(deltaTime);
	return result;
}

const std::string& TrainingSetupPanel::GetSelectedEnemyClass() const
{
	const auto& entries = EnemyCatalog::GetEntries();
	const int32_t index = itemList_.GetSelectedIndex();
	if (index < 0 || index >= static_cast<int32_t>(entries.size())) {
		return kEmptyClassName;
	}
	return entries[index].className;
}

void TrainingSetupPanel::Refresh(float deltaTime)
{
	const bool visible = (state_ != State::Hidden);
	const bool usePad = Input::GetInstance().IsConnected();

	backdrop_->SetColor({ 0.0f, 0.0f, 0.0f, kBackdropAlpha * alpha_ });
	backdrop_->GetRenderState().isVisible = visible;
	backdrop_->Update();

	title_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ });
	title_->GetRenderState().isVisible = visible;
	title_->Update();

	itemList_.Refresh(alpha_, deltaTime);

	// 選択中の敵の説明。項目を動かすたびに入れ替わる
	const auto& entries = EnemyCatalog::GetEntries();
	const int32_t index = itemList_.GetSelectedIndex();
	if (index >= 0 && index < static_cast<int32_t>(entries.size())) {
		description_->SetText(entries[index].description);
	}
	description_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ * 0.75f });
	description_->GetRenderState().isVisible = visible;
	description_->Update();

	hint_->SetText(usePad
		? "↑↓ : SELECT     A : START     B : BACK"
		: "W S : SELECT     SPACE : START     ESC : BACK");
	hint_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ * 0.8f });
	hint_->GetRenderState().isVisible = visible;
	hint_->Update();
}
