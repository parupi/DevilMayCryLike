#define NOMINMAX
#include "ClearUI.h"
#ifdef _DEBUG
#include <imgui.h>
#endif // DEBUG
#include <base/utility/DeltaTime.h>

void ClearUI::Initialize()
{
    // Result
    resultUI_ = std::make_unique<Sprite>();
    resultUI_->Initialize("Result.png");
    resultUI_->SetAnchorPoint({ 0.5f, 0.5f });

    resultDefaultPos_ = { 162.0f, 80.0f };
    resultUI_->SetPosition({ resultDefaultPos_.x, -200.0f });

    // Stage
    stageNumUI_ = std::make_unique<Sprite>();
    stageNumUI_->Initialize("Stage1.png");
    stageNumUI_->SetAnchorPoint({ 0.5f, 0.5f });

    stageDefaultPos_ = { 106.0f, 145.0f };
    stageNumUI_->SetPosition({ stageDefaultPos_.x, -200.0f });

    // Score
    score_ = std::make_unique<Sprite>();
    score_->Initialize("Score.png");
    score_->SetAnchorPoint({ 0.5f, 0.5f });

    scoreDefaultPos_ = { 142.0f, 340.0f };
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

void ClearUI::Draw()
{
	resultUI_->Draw();
	stageNumUI_->Draw();
	score_->Draw();
	scoreUI_->Draw();
	rankUI_->Draw();
}

float ClearUI::EaseOutBack(float t)
{
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;

    return 1.0f + c3 * (t - 1.0f) * (t - 1.0f) * (t - 1.0f)
        + c1 * (t - 1.0f) * (t - 1.0f);
}
