#define NOMINMAX
#include "ClearUI.h"
#ifdef _DEBUG
#endif // DEBUG
#include "Utility/DeltaTime.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"

namespace {
    // ランク画像（Ranks.png）と同じ「黒い文字に白い縁」に寄せる。
    // TextLabel は縁取りを持たないので、白い影をずらして重ねて縁の代わりにする
    const Vector4 kHeadingColor = { 0.05f, 0.05f, 0.05f, 1.0f };
    const Vector4 kHeadingShadowColor = { 1.0f, 1.0f, 1.0f, 0.9f };
    const Vector2 kHeadingShadowOffset = { 3.0f, 3.0f };

    TextLabel* CreateHeading(const std::string& name, const std::string& text, float fontSize, TextAlignX alignX) {
        TextLabel* label = SpriteManager::GetInstance().CreateTextLabel(SpriteLayer::UI, name);
        label->SetText(text);
        label->SetFontSize(fontSize);
        label->SetAlign(alignX, TextAlignY::Middle);
        label->SetColor(kHeadingColor);
        label->SetShadow(true, kHeadingShadowOffset, kHeadingShadowColor);
        return label;
    }
}

void ClearUI::Initialize()
{
    // 位置の y はどれも文字の縦の中心。落ちてくる演出では y だけを動かす

    // Result
    resultUI_ = CreateHeading("result", "Result", 64.0f, TextAlignX::Left);

    resultDefaultPos_ = { 40.0f, 80.0f };
    resultUI_->SetPosition({ resultDefaultPos_.x, -200.0f });

    // Stage
    stageNumUI_ = CreateHeading("stageNum", "Stage1", 40.0f, TextAlignX::Left);

    stageDefaultPos_ = { 44.0f, 145.0f };
    stageNumUI_->SetPosition({ stageDefaultPos_.x, -200.0f });

    // Score（右隣に ScoreUI の数字が並ぶので、右揃えにして数字の手前に収める）
    score_ = CreateHeading("score", "Score :", 48.0f, TextAlignX::Right);

    scoreDefaultPos_ = { 222.0f, 342.0f };
    score_->SetPosition({ scoreDefaultPos_.x, -200.0f });

	scoreUI_ = std::make_unique<ScoreUI>();
	scoreUI_->Initialize();

	rankUI_ = std::make_unique<RankUI>();
	rankUI_->Initialize();
}

void ClearUI::Update()
{
    float dt = DeltaTime::GetDeltaTime();
    timer_ += dt;

    const float animTime = 0.6f;
    float t = std::min(timer_ / animTime, 1.0f);
    float ease = EaseOutBack(t);

    switch (state_) {

    case State::ResultDrop:
        resultUI_->SetPosition({
            resultDefaultPos_.x,
            Lerp(-200.0f, resultDefaultPos_.y, ease)
            });
        resultUI_->Update();

        if (t >= 1.0f) {
            state_ = State::StageDrop;
            timer_ = 0.0f;
        }
        break;

    case State::StageDrop:
        stageNumUI_->SetPosition({
            stageDefaultPos_.x,
            Lerp(-200.0f, stageDefaultPos_.y, ease)
            });
        stageNumUI_->Update();

        if (t >= 1.0f) {
            state_ = State::ScoreDrop;
            timer_ = 0.0f;
        }
        break;

    case State::ScoreDrop:
        score_->SetPosition({
            scoreDefaultPos_.x,
            Lerp(-200.0f, scoreDefaultPos_.y, ease)
            });
        score_->Update();

        if (t >= 1.0f) {
            state_ = State::Finished;
        }
        break;

    case State::Finished:
        resultUI_->Update();
        stageNumUI_->Update();
        score_->Update();

        scoreUI_->Start();
        break;
    }
    if (scoreUI_->isFinished()) {
        rankUI_->Start();
    }

	scoreUI_->Update();

	rankUI_->Update();
}

float ClearUI::EaseOutBack(float t)
{
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;

    return 1.0f + c3 * (t - 1.0f) * (t - 1.0f) * (t - 1.0f)
        + c1 * (t - 1.0f) * (t - 1.0f);
}
