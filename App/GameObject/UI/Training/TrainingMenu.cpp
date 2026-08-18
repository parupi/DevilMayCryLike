#include "TrainingMenu.h"

#include "GameData/EnemyCatalog.h"
#include "GameObject/Training/TrainingController.h"

#include <Audio/SoundManager.h>
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

	/// 行の表示名。TrainingMenu::Row の並びと合わせること
	const char* kRowTexts[] = {
		"ENEMY",
		"ENEMY BEHAVIOR",
		"PLAYER INVINCIBLE",
		"ENEMY INVINCIBLE",
		"AUTO RESPAWN",
		"STATUS HUD",
		"RESPAWN ENEMY",
		"RESET ALL",
		"BACK TO TRAINING",
	};

	const char* OnOff(bool value) { return value ? "ON" : "OFF"; }
}

void TrainingMenu::Initialize()
{
	SpriteManager& sprites = SpriteManager::GetInstance();

	backdrop_ = sprites.CreateSprite(SpriteLayer::UI, "trainMenuBackdrop", "white.png");
	backdrop_->SetAnchorPoint({ 0.5f, 0.5f });
	backdrop_->SetPosition({ kCenterX, kScreenHeight * 0.5f });
	backdrop_->SetSize({ kScreenWidth, kScreenHeight });

	// 選択行の目印。OPTION 画面と同じ見せ方に揃えている
	highlight_ = sprites.CreateSprite(SpriteLayer::UI, "trainMenuHighlight", "circle.png");
	highlight_->SetAnchorPoint({ 0.5f, 0.5f });
	highlight_->SetSize({ 700.0f, 42.0f });
	highlight_->GetRenderState().blendMode = BlendMode::kAdd;

	cursor_ = sprites.CreateSprite(SpriteLayer::UI, "trainMenuCursor", "SelectArrow.png");
	cursor_->SetAnchorPoint({ 0.5f, 0.5f });
	cursor_->SetSize({ 24.0f, 24.0f });

	for (int32_t i = 0; i < kRowCount; ++i) {
		const std::string index = std::to_string(i);
		const float y = kRowStartY + kRowSpacing * i;

		RowSprites& row = rows_[i];

		row.label = sprites.CreateTextLabel(SpriteLayer::UI, "trainMenuLabel" + index);
		row.label->SetText(kRowTexts[i]);
		row.label->SetFontSize(kFontSize);
		row.label->SetAlign(TextAlignX::Left, TextAlignY::Middle);
		row.label->SetPosition({ kLabelX, y });
		row.label->SetShadow(true);

		// 決定で走る行は値を持たないので、ラベルだけで済ませる
		if (IsActionRow(static_cast<Row>(i))) continue;

		row.value = sprites.CreateTextLabel(SpriteLayer::UI, "trainMenuValue" + index);
		row.value->SetFontSize(kFontSize);
		row.value->SetAlign(TextAlignX::Left, TextAlignY::Middle);
		row.value->SetPosition({ kValueX, y });
		row.value->SetShadow(true);
	}

	title_ = sprites.CreateTextLabel(SpriteLayer::UI, "trainMenuTitle");
	title_->SetText("TRAINING");
	title_->SetFontSize(46.0f);
	title_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	title_->SetPosition({ kCenterX, 112.0f });
	title_->SetShadow(true);

	description_ = sprites.CreateTextLabel(SpriteLayer::UI, "trainMenuDescription");
	description_->SetFontSize(22.0f);
	description_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	description_->SetPosition({ kCenterX, kDescriptionY });
	description_->SetShadow(true);

	hint_ = sprites.CreateTextLabel(SpriteLayer::UI, "trainMenuHint");
	hint_->SetFontSize(22.0f);
	hint_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	hint_->SetPosition({ kCenterX, 660.0f });
	hint_->SetShadow(true);

	// 開くまでは何も出さない
	Refresh(nullptr);
}

void TrainingMenu::Open()
{
	state_ = State::Appearing;
	result_ = Result::None;
	selectedIndex_ = 0;
	pulseTimer_ = 0.0f;
}

void TrainingMenu::Close()
{
	if (state_ == State::Hidden) return;
	state_ = State::Closing;
}

void TrainingMenu::Update(TrainingController* controller)
{
	navigator_.Update();

	// 開いている間はシーンの時間が止まっているので、演出は実時間で進める
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

	// 操作を受けるのは出きってから。
	// 開くのに押したキーをそのまま「閉じる」として拾ってしまうのを防ぐ間でもある
	if (state_ == State::Shown && controller) {
		pulseTimer_ += deltaTime;

		if (navigator_.IsUp()) {
			selectedIndex_ = (selectedIndex_ + kRowCount - 1) % kRowCount;
			SoundManager::GetInstance().PlaySE("SwordSlash", 0.25f);
		}
		if (navigator_.IsDown()) {
			selectedIndex_ = (selectedIndex_ + 1) % kRowCount;
			SoundManager::GetInstance().PlaySE("SwordSlash", 0.25f);
		}
		if (navigator_.IsLeft()) ChangeValue(*controller, -1);
		if (navigator_.IsRight()) ChangeValue(*controller, +1);
		if (navigator_.IsDecide()) Decide(*controller);

		// 開けたキーでそのまま閉じられるようにする。
		// 実際に閉じるのは結果を受け取った GameSceneStateTrainingMenu
		const Input& input = Input::GetInstance();
		const bool closePressed = navigator_.IsCancel()
			|| input.TriggerKey(DIK_TAB)
			|| (input.IsConnected() && input.TriggerButton(PadNumber::ButtonBack));
		if (closePressed) {
			result_ = Result::Close;
		}
	}

	Refresh(controller);
}

void TrainingMenu::ChangeValue(TrainingController& controller, int32_t direction)
{
	switch (static_cast<Row>(selectedIndex_)) {
	case Row::Enemy: {
		const int32_t count = static_cast<int32_t>(EnemyCatalog::GetEntries().size());
		if (count <= 0) return;
		// 端で止める。押しっぱなしのリピートで一覧を往復させないため
		const int32_t next = std::clamp(controller.GetEnemyIndex() + direction, 0, count - 1);
		if (next == controller.GetEnemyIndex()) return;
		// 切り替えると、その場で相手が出し直される
		controller.SelectEnemy(next);
		break;
	}
	case Row::Behavior: {
		const int32_t count = static_cast<int32_t>(TrainingBehavior::Count);
		const int32_t current = static_cast<int32_t>(controller.GetBehavior());
		const int32_t next = std::clamp(current + direction, 0, count - 1);
		if (next == current) return;
		controller.SetBehavior(static_cast<TrainingBehavior>(next));
		break;
	}
	// ON/OFF は左で OFF・右で ON にする。
	// 押すたびに反転させると、押しっぱなしのリピートで往復してしまう
	case Row::PlayerInvincible:
		controller.SetPlayerInvincible(direction > 0);
		break;
	case Row::EnemyInvincible:
		controller.SetEnemyInvincible(direction > 0);
		break;
	case Row::AutoRespawn:
		controller.SetAutoRespawn(direction > 0);
		break;
	case Row::Hud:
		controller.SetHudVisible(direction > 0);
		break;
	default:
		// 決定で走る行に左右は無い
		return;
	}

	SoundManager::GetInstance().PlaySE("SwordSlash", 0.3f);
}

void TrainingMenu::Decide(TrainingController& controller)
{
	SoundManager::GetInstance().PlaySE("SwordHit", 0.6f);

	switch (static_cast<Row>(selectedIndex_)) {
	// 値の行は1つ先へ進める。端まで行ったら先頭へ戻すので、決定だけでも一周できる
	case Row::Enemy:
		controller.SelectNextEnemy();
		break;
	case Row::Behavior:
		controller.CycleBehavior();
		break;
	case Row::PlayerInvincible:
		controller.SetPlayerInvincible(!controller.IsPlayerInvincible());
		break;
	case Row::EnemyInvincible:
		controller.SetEnemyInvincible(!controller.IsEnemyInvincible());
		break;
	case Row::AutoRespawn:
		controller.SetAutoRespawn(!controller.IsAutoRespawn());
		break;
	case Row::Hud:
		controller.SetHudVisible(!controller.IsHudVisible());
		break;
	case Row::Respawn:
		controller.RequestRespawn();
		break;
	case Row::Reset:
		controller.ResetAll();
		break;
	case Row::Back:
		// 閉じるのは結果を受け取った GameSceneStateTrainingMenu
		result_ = Result::Close;
		break;
	default:
		break;
	}
}

void TrainingMenu::Refresh(const TrainingController* controller)
{
	const bool visible = (state_ != State::Hidden);
	const bool usePad = Input::GetInstance().IsConnected();

	backdrop_->SetColor({ 0.0f, 0.0f, 0.0f, kBackdropAlpha * alpha_ });
	backdrop_->GetRenderState().isVisible = visible;
	backdrop_->Update();

	title_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ });
	title_->GetRenderState().isVisible = visible;
	title_->Update();

	// 選択行の目印。ゆっくり明滅させて、どこを触っているか分かるようにする
	const float pulse = 0.5f + 0.5f * std::cos(pulseTimer_ * 3.0f);
	const float selectedY = kRowStartY + kRowSpacing * selectedIndex_;

	highlight_->SetPosition({ 660.0f, selectedY });
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

		if (!row.value) continue;

		// 相手が未生成のフレームもあるので、値は controller が無ければ伏せる
		std::string text = "-";
		if (controller) {
			switch (static_cast<Row>(i)) {
			case Row::Enemy:            text = controller->GetEnemyDisplayName(); break;
			case Row::Behavior:         text = controller->GetBehaviorLabel(); break;
			case Row::PlayerInvincible: text = OnOff(controller->IsPlayerInvincible()); break;
			case Row::EnemyInvincible:  text = OnOff(controller->IsEnemyInvincible()); break;
			case Row::AutoRespawn:      text = OnOff(controller->IsAutoRespawn()); break;
			case Row::Hud:              text = OnOff(controller->IsHudVisible()); break;
			default: break;
			}
		}

		row.value->SetText(text);
		row.value->SetColor({ brightness, brightness, brightness, alpha_ });
		row.value->GetRenderState().isVisible = visible;
		row.value->Update();
	}

	// 選択中の敵の説明。相手を切り替えるたびに入れ替わる
	if (controller) {
		const std::vector<EnemyCatalogEntry>& entries = EnemyCatalog::GetEntries();
		const int32_t index = controller->GetEnemyIndex();
		if (index >= 0 && index < static_cast<int32_t>(entries.size())) {
			description_->SetText(entries[index].description);
		}
	}
	description_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ * 0.75f });
	description_->GetRenderState().isVisible = visible;
	description_->Update();

	// 矢印は記号のまま出す（フォントが Noto Sans JP なので↑↓←→も持っている）
	hint_->SetText(usePad
		? "↑↓ : SELECT     ←→ : CHANGE     A : DECIDE     BACK : CLOSE"
		: "W S : SELECT     A D : CHANGE     SPACE : DECIDE     TAB : CLOSE");
	hint_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ * 0.8f });
	hint_->GetRenderState().isVisible = visible;
	hint_->Update();
}
