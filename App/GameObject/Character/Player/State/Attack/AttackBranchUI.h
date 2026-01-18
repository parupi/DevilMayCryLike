#pragma once
#pragma once
#include <vector>
#include "2D/Sprite.h"
#include "GameObject/Character/CharacterStructs.h"
#include "PlayerStateAttack.h"
#include "AttackStructs.h"

class AttackBranchUI
{
public:
    void Initialize();
    void SetVisible(bool visible) { isVisible_ = visible; }

    void SetBranches(const std::vector<AttackBranch>& branches);
    void Clear();

    void SetCurrentInput(InputType input, StickDir dir, bool isLockOn);

    void Update();

    void Draw();

private:
    struct BranchSprites
    {
        AttackBranch branch;

        std::unique_ptr<Sprite> y;
        std::unique_ptr<Sprite> plus1;
        std::unique_ptr<Sprite> rb;
        std::unique_ptr<Sprite> plus2;
        std::unique_ptr<Sprite> stick;
        std::unique_ptr<Sprite> arrow;
        std::unique_ptr<Sprite> attack;
    };

    std::vector<BranchSprites> branches_;

    bool isVisible_ = false;

    InputType currentInput_;
    StickDir currentDir_;
    bool currentLockOn_ = false;

    Vector2 origin_{ 40.0f, 40.0f };
    float lineHeight_ = 48.0f;
};