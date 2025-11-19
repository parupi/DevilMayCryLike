#define NOMINMAX
#include "RankUI.h"
#include <base/utility/DeltaTime.h>
#include <GameData/GameData.h>

void RankUI::Initialize()
{
    rank_ = std::make_unique<Sprite>();
    rank_->Initialize("Ranks.png");
    rank_->SetUVSize({ 0.2f, 1.0f });
    rank_->SetAnchorPoint({ 0.5f, 0.5f });

    // ランクに応じた UV の X 座標
    float uvX = 0.0f; // default D

    std::string currentRank = GameData::GetInstance()->GetClearRank();
        
    if (currentRank == "C")      uvX = 0.2f;
    else if (currentRank == "B") uvX = 0.4f;
    else if (currentRank == "A") uvX = 0.6f;
    else if (currentRank == "S" || currentRank == "SS" || currentRank == "SSS")
        uvX = 0.8f;

    // UV 適用
    rank_->SetUVPosition({ uvX, 0.0f });

    // 初期は上に配置 & 大きめ
    rank_->SetPosition(startPos_);
    rank_->SetSize(startSize_);
}

void RankUI::Update()
{
    if (!isStart_) return;

    timer_ += DeltaTime::GetDeltaTime();  // フレーム時間加算（仮定）

    // t を 0 → 1 にクランプ
    float t = std::min(timer_ * 1.2f, 1.0f);

    // -------------------------
    // 1. 落下する動き（EaseOutBounce）
    // -------------------------
    auto EaseOutBounce = [](float x)
        {
            if (x < 1 / 2.75f) {
                return 7.5625f * x * x;
            } else if (x < 2 / 2.75f) {
                x -= 1.5f / 2.75f;
                return 7.5625f * x * x + 0.75f;
            } else if (x < 2.5 / 2.75f) {
                x -= 2.25f / 2.75f;
                return 7.5625f * x * x + 0.9375f;
            } else {
                x -= 2.625f / 2.75f;
                return 7.5625f * x * x + 0.984375f;
            }
        };

    float ty = EaseOutBounce(t);  // 0→1 に対してバウンドする値

    // 線形補間
    float posY = startPos_.y * (1 - ty) + targetPos_.y * ty;
    float posX = targetPos_.x;

    rank_->SetPosition({ posX, posY });


    // -------------------------
    // 2. サイズ補間（大きい→本来の大きさ）
    // -------------------------
    auto Lerp = [](float a, float b, float t) { return a + (b - a) * t; };

    float sizeX = Lerp(startSize_.x, targetSize_.x, ty);
    float sizeY = Lerp(startSize_.y, targetSize_.y, ty);

    rank_->SetSize({ sizeX, sizeY });

    // 最後に更新
    rank_->Update();
}


void RankUI::Draw()
{
	rank_->Draw();
}

void RankUI::Start()
{
    // スタートしてたら抜ける
    if (isStart_) return;

    isStart_ = true;
    timer_ = 0.0f;

    // 初期位置とサイズをリセット
    rank_->SetPosition(startPos_);
    rank_->SetSize(startSize_);
}
