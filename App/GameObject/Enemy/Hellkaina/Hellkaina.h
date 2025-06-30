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

private:


    
};

