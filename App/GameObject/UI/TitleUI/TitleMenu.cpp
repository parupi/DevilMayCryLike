#include "TitleMenu.h"

#include <Audio/SoundManager.h>
#include <Graphics/Rendering/Sprite/SpriteManager.h>
#include <Graphics/Resource/TextureManager.h>
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

void TitleMenu::LoadTextures()
{
	// 文字はフォントから描くので、ここで要るのは下敷きに使う絵だけ
	TextureManager::GetInstance().LoadTexture("white.png");
	TextureManager::GetInstance().LoadTexture("circle.png");
	TextureManager::GetInstance().LoadTexture("SelectArrow.png");
}

void TitleMenu::Initialize()
{
	itemList_.Initialize("titleMenu", SpriteLayer::UI,
		{ "GAME START", "TRAINING", "CONTROLS", "OPTION", "QUIT" },
		{ kCenterX, kItemStartY }, kItemSpacing, kItemFontSize);

	hint_ = SpriteManager::GetInstance().CreateTextLabel(SpriteLayer::UI, "titleMenuHint");
	hint_->SetFontSize(22.0f);
	hint_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	hint_->SetPosition({ kCenterX, 664.0f });
	hint_->SetShadow(true);

	controlsPanel_ = std::make_unique<ControlsPanel>();
	controlsPanel_->Initialize();

	optionPanel_ = std::make_unique<OptionPanel>();
	optionPanel_->Initialize();

	trainingPanel_ = std::make_unique<TrainingSetupPanel>();
	trainingPanel_->Initialize();

	// 確認ダイアログは一覧より後に作る（同じレイヤーでは後から作ったものが手前に出る）
	confirmDialog_.Initialize("titleMenu", SpriteLayer::UI);

	// 開くまでは何も出さない
	RefreshRoot();
	confirmDialog_.Update(navigator_);
}

void TitleMenu::Open()
{
	if (phase_ != Phase::Hidden) return;

	phase_ = Phase::Root;
	itemList_.SetSelectedIndex(0);
	result_ = Result::None;
}

void TitleMenu::Close()
{
	if (phase_ == Phase::Hidden) return;

	controlsPanel_->Close();
	optionPanel_->Close();
	trainingPanel_->Close();
	confirmDialog_.Close();
	phase_ = Phase::Closing;
}

void TitleMenu::Update()
{
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

	// 敵の選択は決定・キャンセルを戻り値で返してくる。
	// 閉じるアニメを進めるため、開いていないフレームでも呼ぶ
	const TrainingSetupPanel::Result trainingResult = trainingPanel_->Update(navigator_);
	if (phase_ == Phase::Training) {
		if (trainingResult == TrainingSetupPanel::Result::Decided) {
			// 選ばれた敵の受け渡しとシーンの切り替えは TitleScene が受け持つ
			result_ = Result::StartTraining;
			phase_ = Phase::Closing;
		} else if (trainingResult == TrainingSetupPanel::Result::Canceled) {
			trainingPanel_->Close();
			phase_ = Phase::Root;
		}
	}

	// 終了が決まったあとも、暗転しきるまでは出したままにする
	const ConfirmDialog::Answer answer = confirmDialog_.Update(navigator_);
	if (phase_ == Phase::Confirm) {
		if (answer == ConfirmDialog::Answer::Yes) {
			// 暗転とアプリの終了は TitleScene が受け持つ
			result_ = Result::Quit;
			phase_ = Phase::Closing;
		} else if (answer == ConfirmDialog::Answer::No) {
			confirmDialog_.Close();
			phase_ = Phase::Root;
		}
	}

	RefreshRoot();

	// 一覧が消えきったら完全に閉じる（終了を選んだ場合は暗転に任せて出したままにする）
	if (phase_ == Phase::Closing && rootAlpha_ <= 0.0f && result_ != Result::Quit) {
		phase_ = Phase::Hidden;
	}
}

void TitleMenu::UpdateRoot()
{
	// 出きるまでは操作を受けない。
	// 子パネルから戻った直後の入力を拾ってしまわないための間でもある
	if (rootAlpha_ < 1.0f) return;

	itemList_.UpdateSelection(navigator_);

	if (navigator_.IsDecide()) {
		Decide();
	}
}

void TitleMenu::Decide()
{
	switch (static_cast<Item>(itemList_.GetSelectedIndex())) {
	case Item::GameStart:
		SoundManager::GetInstance().PlaySE("SwordHit", 0.6f);
		// 実際にシーンを進めるのは TitleScene
		result_ = Result::StartGame;
		break;
	case Item::Training:
		SoundManager::GetInstance().PlaySE("SwordHit", 0.6f);
		trainingPanel_->Open();
		phase_ = Phase::Training;
		break;
	case Item::Controls:
		SoundManager::GetInstance().PlaySE("SwordHit", 0.6f);
		controlsPanel_->Open();
		phase_ = Phase::Controls;
		break;
	case Item::Option:
		SoundManager::GetInstance().PlaySE("SwordHit", 0.6f);
		optionPanel_->Open();
		phase_ = Phase::Option;
		break;
	case Item::Quit:
		SoundManager::GetInstance().PlaySE("SwordHit", 0.6f);
		// 押し間違いでアプリが落ちないよう、必ず一段挟む
		confirmDialog_.Open("QUIT THE GAME?");
		phase_ = Phase::Confirm;
		break;
	default:
		break;
	}
}

const std::string& TitleMenu::GetSelectedTrainingEnemy() const
{
	return trainingPanel_->GetSelectedEnemyClass();
}

void TitleMenu::RefreshRoot()
{
	const float deltaTime = DeltaTime::GetUnscaledDeltaTime();
	const float step = (kFadeTime > 0.0f) ? (deltaTime / kFadeTime) : 1.0f;

	// 子パネルや確認ダイアログを開いている間は一覧を引っ込める。
	// 残したままだと、確認の文言と項目が重なって読みにくい
	const bool showRoot = (phase_ == Phase::Root);
	rootAlpha_ = MoveToward(rootAlpha_, showRoot ? 1.0f : 0.0f, step);

	itemList_.Refresh(rootAlpha_, deltaTime);

	const bool usePad = Input::GetInstance().IsConnected();
	hint_->SetText(usePad ? "A : DECIDE     B : BACK" : "SPACE : DECIDE     ESC : BACK");
	hint_->SetColor({ 1.0f, 1.0f, 1.0f, rootAlpha_ * 0.8f });
	hint_->GetRenderState().isVisible = rootAlpha_ > 0.0f;
	hint_->Update();
}
