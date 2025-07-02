#pragma once
#include <memory>
#include "GameObject/Enemy/Enemy.h"
class Hellkaina : public Enemy
{
public:
    Hellkaina(std::string objectName);
    // 初期化
    void Initialize() override;
    // 更新
    void Update() override;
    // 

        // 衝突系
    void OnCollisionEnter([[maybe_unused]] BaseCollider* other) override;
    void OnCollisionStay([[maybe_unused]] BaseCollider* other) override;
    void OnCollisionExit([[maybe_unused]] BaseCollider* other) override;
private:


    
};

