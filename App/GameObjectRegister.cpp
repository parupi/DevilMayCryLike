#include "GameObjectRegister.h"
#include "scene/Object3dFactory.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Character/Enemy/Hellkaina/Hellkaina.h"
#include "GameObject/Character/Enemy/GruntMelee/GruntMelee.h"
#include "GameObject/Character/Enemy/BossKnight/BossKnight.h"
#include "GameObject/Ground/Ground.h"

void RegisterAllGameObjects() {
    Object3dFactory::Register("Player",      [](const std::string& n){ return std::make_unique<Player>(n);      });
    Object3dFactory::Register("HellKaina",   [](const std::string& n){ return std::make_unique<Hellkaina>(n);   });
    Object3dFactory::Register("GruntMelee",  [](const std::string& n){ return std::make_unique<GruntMelee>(n);  });
    Object3dFactory::Register("BossKnight",  [](const std::string& n){ return std::make_unique<BossKnight>(n);  });
    Object3dFactory::Register("Ground",      [](const std::string& n){ return std::make_unique<Ground>(n);      });
}
