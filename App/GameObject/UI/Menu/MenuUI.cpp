#include "MenuUI.h"
#include "Scene/GameScene/GameScene.h"

#include <Audio/SoundManager.h>
#include <Graphics/Rendering/Sprite/SpriteManager.h>
#include <Input/Input.h>
#include <Utility/DeltaTime.h>

#include <algorithm>

namespace {
	constexpr float kCenterX = 640.0f;

	// 目標値へ一定の速さで寄せる（0～1のアルファ用）。
	// Windows.h の min / max マクロと衝突するので、括弧で囲って呼ぶ
	float MoveToward(float current, float target, float step) {
		if (current < target) return (std::min)(current + step, target);
		return (std::max)(current - step, target);
	}
}

void MenuUI::Initialize(GameScene* scene) {
	scene_ = scene;

	divider_ = std::make_unique<MenuDivider>();
	divider_->Initialize();

	title_ = SpriteManager::GetInstance().CreateTextLabel(SpriteLayer::UI, "pauseTitle");
	title_->SetText("PAUSE");
	title_->SetFontSize(46.0f);
	title_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	title_->SetPosition({ kCenterX, kTitleY });
	title_->SetShadow(true);

	itemList_.Initialize("pauseMenu", SpriteLayer::UI,
		{ "CONTINUE", "RETRY", "CONTROLS", "OPTION", "TO TITLE" },
		{ kCenterX, kItemStartY }, kItemSpacing, kItemFontSize);

	hint_ = SpriteManager::GetInstance().CreateTextLabel(SpriteLayer::UI, "pauseHint");
	hint_->SetFontSize(22.0f);
	hint_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	hint_->SetPosition({ kCenterX, kHintY });
	hint_->SetShadow(true);

	controlsPanel_ = std::make_unique<ControlsPanel>();
	controlsPanel_->Initialize();

	optionPanel_ = std::make_unique<OptionPanel>();
	optionPanel_->Initialize();

	// 確認ダイアログは一覧より後に作る（同じレイヤーでは後から作ったものが手前に出る）
	confirmDialog_.Initialize("pauseMenu", SpriteLayer::UI);

	// 開くまでは何も出さない
	RefreshRoot();
	confirmDialog_.Update(navigator_);
}

void MenuUI::Enter() {
	phase_ = Phase::Root;
	result_ = Result::None;
	pendingResult_ = Result::None;
	itemList_.SetSelectedIndex(0);
	divider_->Enter();
}

void MenuUI::Exit() {
	if (phase_ == Phase::Hidden) return;

	controlsPanel_->Close();
	optionPanel_->Close();
	confirmDialog_.Close();
	divider_->Exit();
	phase_ = Phase::Closing;
}

void MenuUI::Update() {
	navigator_.Update();

	switch (phase_) {
	case Phase::Root:
		UpdateRoot();
		break;
	case Phase::Controls:
		// 操作説明は読むだけなので、どちらのボタンでも一覧へ戻す
		if (navigator_.IsCancel() || navigator_.IsDecide()) {
			controlsPanel_->Close();
			phase_ = Phase::Root;
		}
		break;
	case Phase::Option:
		if (navigator_.IsCancel()) {
			// Close の中で設定がファイルへ書き出される
			optionPanel_->Close();
			phase_ = Phase::Root;
		}
		break;
	default:
		break;
	}

	controlsPanel_->Update();
	optionPanel_->Update(navigator_);

	// 確認の結果。「はい」なら、暗転しきるまで出したままにしておく
	const ConfirmDialog::Answer answer = confirmDialog_.Update(navigator_);
	if (phase_ == Phase::Confirm) {
		if (answer == ConfirmDialog::Answer::Yes) {
			result_ = pendingResult_;
			phase_ = Phase::Closing;
		} else if (answer == ConfirmDialog::Answer::No) {
			confirmDialog_.Close();
			phase_ = Phase::Root;
		}
	}

	RefreshRoot();
	divider_->Update();

	// 閉じきったらゲームへ戻す。やり直し・タイトルへの場合は
	// シーンの切り替えに任せるので、ここでは何もしない
	if (phase_ == Phase::Closing && rootAlpha_ <= 0.0f && result_ == Result::None) {
		phase_ = Phase::Hidden;
		result_ = Result::Resume;
	}
}

void MenuUI::UpdateRoot() {
	// 出きるまでは操作を受けない。
	// 子パネルから戻った直後の入力を拾ってしまわないための間でもある
	if (rootAlpha_ < 1.0f) return;

	// メニューを開いたボタンでそのまま閉じられるようにする
	const Input& input = Input::GetInstance();
	if (input.TriggerKey(DIK_M) || input.TriggerButton(ButtonStart) || navigator_.IsCancel()) {
		SoundManager::GetInstance().PlaySE("SwordSlash", 0.3f);
		Exit();
		return;
	}

	itemList_.UpdateSelection(navigator_);

	if (navigator_.IsDecide()) {
		Decide();
	}
}

void MenuUI::Decide() {
	SoundManager::GetInstance().PlaySE("SwordHit", 0.6f);

	switch (static_cast<Item>(itemList_.GetSelectedIndex())) {
	case Item::Continue:
		Exit();
		break;
	case Item::Retry:
		// ここまでの進行が消えるので確認を挟む
		pendingResult_ = Result::Retry;
		confirmDialog_.Open("RETRY FROM THE START?");
		phase_ = Phase::Confirm;
		break;
	case Item::Controls:
		controlsPanel_->Open();
		phase_ = Phase::Controls;
		break;
	case Item::Option:
		optionPanel_->Open();
		phase_ = Phase::Option;
		break;
	case Item::ToTitle:
		pendingResult_ = Result::ToTitle;
		confirmDialog_.Open("RETURN TO TITLE?");
		phase_ = Phase::Confirm;
		break;
	default:
		break;
	}
}

void MenuUI::RefreshRoot() {
	// ゲームは止まっているので、メニューの演出は実時間で動かす
	const float deltaTime = DeltaTime::GetUnscaledDeltaTime();
	const float step = (kFadeTime > 0.0f) ? (deltaTime / kFadeTime) : 1.0f;

	// 子パネルや確認ダイアログを開いている間は一覧を引っ込める。
	// 残したままだと、確認の文言と項目が重なって読みにくい
	const bool showRoot = (phase_ == Phase::Root);
	rootAlpha_ = MoveToward(rootAlpha_, showRoot ? 1.0f : 0.0f, step);

	itemList_.Refresh(rootAlpha_, deltaTime);

	const bool visible = rootAlpha_ > 0.0f;

	title_->SetColor({ 1.0f, 1.0f, 1.0f, rootAlpha_ });
	title_->GetRenderState().isVisible = visible;
	title_->Update();

	const bool usePad = Input::GetInstance().IsConnected();
	hint_->SetText(usePad ? "A : DECIDE     B : RESUME" : "SPACE : DECIDE     ESC : RESUME");
	hint_->SetColor({ 1.0f, 1.0f, 1.0f, rootAlpha_ * 0.8f });
	hint_->GetRenderState().isVisible = visible;
	hint_->Update();
}
