#include "StylishScoreManager.h"
#include <base/utility/DeltaTime.h>
#include <imgui.h>

void StylishScoreManager::Update()
{
    timeSinceLastAction_ += DeltaTime::GetDeltaTime();

    // 時間経過で減衰
    if (timeSinceLastAction_ > 3.0f) {
        currentScore_ = std::max(0, currentScore_ - static_cast<int32_t>(100 * DeltaTime::GetDeltaTime()));
        UpdateRank();
    }

    ImGui::Begin("Stylish");

    ImGui::Text("Current Rank: %s", GetCurrentRank().c_str());
    ImGui::Text("Current Score: %d", GetCurrentScore());

    ImGui::End();
}

void StylishScoreManager::AddScore(int32_t scoreNum, std::string attackName)
{
    if (scoreNum > 0) {
        currentScore_ += scoreNum;
        timeSinceLastAction_ = 0.0f;
        UpdateRank();
    }
}

void StylishScoreManager::OnDamage()
{
}

void StylishScoreManager::UpdateRank()
{
    std::string newRank = "D";

    if (currentScore_ >= 2200) newRank = "SSS";
    else if (currentScore_ >= 1500) newRank = "SS";
    else if (currentScore_ >= 1000) newRank = "S";
    else if (currentScore_ >= 600) newRank = "A";
    else if (currentScore_ >= 300) newRank = "B";
    else if (currentScore_ >= 100) newRank = "C";

    if (newRank != currentRank_) {
        currentRank_= newRank;
    }
}
