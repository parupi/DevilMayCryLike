#include "Object3dFactory.h"
#include "GameObject/Enemy/Enemy.h"
#include "GameObject/Enemy/Hellkaina/Hellkaina.h"
#include "GameObject/Player/Player.h"
#include "GameObject/Ground/Ground.h"

std::unique_ptr<Object3d> Object3dFactory::Create(const std::string& className, const std::string& objectName) {
    if (className == "Player") {
        return std::make_unique<Player>(objectName);
    } else if (className == "HellKaina") {
        return std::make_unique<Hellkaina>(objectName);
    } else if (className == "Ground") {
        return std::make_unique<Ground>(objectName);
    } else {
        return std::make_unique<Object3d>(objectName); // デフォルト
    }
}
