#include "EnemySensorComponent.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Player/Player.h"

void EnemySensorComponent::Update(Enemy& enemy)
{
    Player* player = enemy.GetPlayer();
    if (!player) {
        detected_  = false;
        distance_  = 0.0f;
        direction_ = {};
        return;
    }

    Vector3 toPlayer = player->GetWorldTransform()->GetTranslation() - enemy.GetWorldTransform()->GetTranslation();
    distance_  = Length(toPlayer);
    direction_ = (distance_ > 0.0001f) ? Normalize(toPlayer) : Vector3{};

    if (!detected_ && distance_ <= detectionRange_) {
        detected_ = true;
    }
}
