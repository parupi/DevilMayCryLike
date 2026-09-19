#include "Tutorial.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include "Graphics/Text/TextLabel.h"
#include "Input/Input.h"

namespace {
	// 暗幕（TutorialDecoration）の中央に合わせる。位置を変えるならあちらの定数も一緒に見ること
	constexpr float kTextCenterX = 210.0f;
	// 1行目: 操作（小さめ）、2行目: 何の操作か（大きめ）。2行で暗幕の中央に収まる位置
	constexpr float kCommandY = 328.0f;
	constexpr float kTitleY = 386.0f;
	constexpr float kCommandFontSize = 26.0f;
	constexpr float kTitleFontSize = 42.0f;
	// 1フレームぶんのフェード量
	constexpr float kFadeSpeed = 0.01f;
}

void Tutorial::Initialize(const std::string& id, const TutorialText& text, uint32_t maxCounter) {
	// チュートリアルの初期化処理
	state_ = State::Inactive;
	// 進行度の最大値を設定
	maxCounter_ = maxCounter;
	text_ = text;
	alpha_ = 0.0f;

	// 説明文。以前は手順の GIF ＋ 文字入りの PNG だったが、
	// 絵をやめて文字だけにした（操作の表記をパッド／キーボードで出し分けるためフォントから描く）
	SpriteManager& sprites = SpriteManager::GetInstance();

	commandLabel_ = sprites.CreateTextLabel(SpriteLayer::UI, "TutorialCommand" + id);
	commandLabel_->SetFontSize(kCommandFontSize);
	commandLabel_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	commandLabel_->SetPosition({kTextCenterX, kCommandY});
	commandLabel_->SetShadow(true);

	titleLabel_ = sprites.CreateTextLabel(SpriteLayer::UI, "TutorialTitle" + id);
	titleLabel_->SetText(text_.title);
	titleLabel_->SetFontSize(kTitleFontSize);
	titleLabel_->SetAlign(TextAlignX::Center, TextAlignY::Middle);
	titleLabel_->SetPosition({kTextCenterX, kTitleY});
	titleLabel_->SetShadow(true);

	// 始まるまでは何も出さない
	UpdateText(0.0f);
}

void Tutorial::Update() {
	switch (state_) {
	case State::Inactive:
		// 非アクティブな状態の処理
		break;
	case State::Start:
		// 開始中の処理
		// 濃さを徐々に増加させる
		alpha_ += kFadeSpeed;
		if (alpha_ >= 1.0f) {
			alpha_ = 1.0f; // 最大値を超えないようにする
			state_ = State::Active; // アクティブな状態に遷移
		}
		break;
	case State::Active:
		// アクティブな状態の処理
		break;
	case State::End:
		// 終了中の処理
		// 濃さを徐々に減少させる
		alpha_ -= kFadeSpeed;
		if (alpha_ <= 0.0f) {
			alpha_ = 0.0f; // 最小値を下回らないようにする
			state_ = State::Inactive; // 非アクティブな状態に遷移
		}
		break;
	}

	// 説明文を今の濃さで出し入れする
	UpdateText(alpha_);
}

void Tutorial::UpdateText(float alpha) {
	// PlayerInput と同じく「パッドが繋がっていればパッドの操作」で出し分ける。
	// 挿し直されてもすぐ追従できるよう毎フレーム見る（文字列が変わらなければ組み直さない）
	const bool usePad = Input::GetInstance().IsConnected();
	// 消えている間は描画自体を止める
	const bool visible = alpha > 0.0f;

	commandLabel_->SetText(usePad ? text_.padCommand : text_.keyboardCommand);
	// 操作の側は少し落として、何の操作かとの主従を付ける（ControlsPanel の割り当て表記と同じ色）
	commandLabel_->SetColor({0.75f, 0.82f, 0.92f, alpha});
	commandLabel_->GetRenderState().isVisible = visible;
	commandLabel_->Update();

	titleLabel_->SetColor({1.0f, 1.0f, 1.0f, alpha});
	titleLabel_->GetRenderState().isVisible = visible;
	titleLabel_->Update();
}

void Tutorial::Start() {
	state_ = State::Start;
	// 進行度を戻しておく
	counter_ = 0;
}

void Tutorial::End() {
	state_ = State::End;
}

bool Tutorial::StepTutorial() {
	// 進行度を加算
	counter_++;
	// 最大以上ならTrue
	if (counter_ >= maxCounter_) {
		return true;
	}
	return false;
}
