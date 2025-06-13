#include "Object3dFactory.h"
#include "GameObject/Enemy/Enemy.h"
#include "GameObject/Player/Player.h"
#include "GameObject/Ground/Ground.h"

Object3d* Object3dFactory::Create(const std::string& className, const std::string& objectName) {
    if (className == "Player") {
        return new Player(objectName);
    } else if (className == "Enemy") {
        return new Enemy(objectName);
    } else if (className == "Ground") {
        return new Ground(objectName);
    } else {
        return new Object3d(objectName); // デフォルト
    }
}
