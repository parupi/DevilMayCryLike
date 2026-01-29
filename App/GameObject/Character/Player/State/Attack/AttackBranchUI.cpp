#include "AttackBranchUI.h"

void AttackBranchUI::Initialize()
{
}

static std::unique_ptr<Sprite> CreateSprite(const std::string& file)
{
    auto s = std::make_unique<Sprite>();
    s->Initialize(file);
    return s;
}

void AttackBranchUI::SetBranches(const std::vector<AttackBranch>& branches)
{
    Clear();

    for (size_t i = 0; i < branches.size(); ++i)
    {
        BranchSprites ui{};
        ui.branch = branches[i];

        ui.y = CreateSprite("UI/YButton.png");
        ui.plus1 = CreateSprite("UI/plus.png");

        if (ui.branch.requireLockOn)
        {
            ui.rb = CreateSprite("UI/RBButton.png");
            ui.plus2 = CreateSprite("UI/plus.png");
        }

        switch (ui.branch.dir)
        {
        case StickDir::Down:
            ui.stick = CreateSprite("UI/Arrow.png");
            break;
        default:
            break;
        }

        ui.arrow = CreateSprite("UI/Arrow.png");
        //ui.attack = CreateSprite(ui.branch.iconFile); // 攻撃アイコン

        branches_.push_back(std::move(ui));
    }
}

void AttackBranchUI::Update()
{
    if (!isVisible_) return;

    for (size_t i = 0; i < branches_.size(); ++i)
    {
        auto& ui = branches_[i];
        Vector2 pos = {
            origin_.x,
            origin_.y + lineHeight_ * static_cast<float>(i)
        };

        bool matched =
            (ui.branch.input == currentInput_) &&
            (ui.branch.dir == currentDir_) &&
            (!ui.branch.requireLockOn || currentLockOn_);

        Vector4 color = matched ? Vector4{ 1,1,1,1 } : Vector4{ 0.5f,0.5f,0.5f,1 };

        auto setSprite = [&](std::unique_ptr<Sprite>& s)
            {
                if (!s) return;
                s->SetPosition(pos);
                s->SetColor(color);
                s->Update();
                pos.x += 24.0f;
            };

        setSprite(ui.y);
        setSprite(ui.plus1);
        setSprite(ui.rb);
        setSprite(ui.plus2);
        setSprite(ui.stick);
        setSprite(ui.arrow);
        //setSprite(ui.attack);
    }
}

void AttackBranchUI::Draw()
{
    //for (auto& sprite : branches_) {
    //    sprite.y->Draw();
    //    sprite.plus1->Draw();
    //    sprite.rb->Draw();
    //    sprite.plus2->Draw();
    //    sprite.stick->Draw();
    //    sprite.arrow->Draw();
    //    //sprite.y->Draw();
    //}
}

void AttackBranchUI::Clear()
{
    if (!branches_.empty()) {
        branches_.clear();
    }
}

void AttackBranchUI::SetCurrentInput(InputType input, StickDir dir, bool isLockOn)
{
    currentInput_ = input;
    currentDir_ = dir;
    currentLockOn_ = isLockOn;
}