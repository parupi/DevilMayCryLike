#include "GameObjectRegister.h"
#include "Scene/Object3dFactory.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Character/Enemy/GruntMelee/GruntMelee.h"
#include "GameObject/Character/Enemy/BossKnight/BossKnight.h"
#include "GameObject/Character/Enemy/TutorialDummy/TutorialDummy.h"
#include "GameObject/Ground/Ground.h"
#include "GameObject/Prop/Prop.h"
#include "GameObject/Prop/WallTorch.h"
#include "GameObject/Light/StagePointLight.h"
#include "GameObject/Event/BossSpawnEvent.h"
#include "GameObject/Event/ClearEvent.h"
#include "GameObject/Event/EnemySpawnEvent.h"
#include "GameObject/Event/ForceBattleEvent.h"

void RegisterAllGameObjects() {
	// 第3引数は「モデル名を使うクラスか」。エディタの生成ダイアログがモデル選択を出すか判断する
	Object3dFactory::Register("Object3d", [](const std::string& n) { return std::make_unique<Object3d>(n); });
	Object3dFactory::Register("Player", [](const std::string& n) { return std::make_unique<Player>(n); });
	Object3dFactory::Register("GruntMelee", [](const std::string& n) { return std::make_unique<GruntMelee>(n); });
	Object3dFactory::Register("BossKnight", [](const std::string& n) { return std::make_unique<BossKnight>(n); });
	Object3dFactory::Register("TutorialDummy", [](const std::string& n) { return std::make_unique<TutorialDummy>(n); });
	Object3dFactory::Register("Ground", [](const std::string& n) { return std::make_unique<Ground>(n); }, true);
	Object3dFactory::Register("Prop", [](const std::string& n) { return std::make_unique<Prop>(n); }, true);
	Object3dFactory::Register("WallTorch", [](const std::string& n) { return std::make_unique<WallTorch>(n); }, true);
	Object3dFactory::Register("PointLight", [](const std::string& n) { return std::make_unique<StagePointLight>(n); });

	// イベントも BaseEvent : Object3d なので同じファクトリに乗せる。
	// EventManager への登録は BaseEvent のコンストラクタが自分で行う
	Object3dFactory::Register("Event_EnemySpawn", [](const std::string& n) { return std::make_unique<EnemySpawnEvent>(n); });
	Object3dFactory::Register("Event_ForceBattle", [](const std::string& n) { return std::make_unique<ForceBattleEvent>(n); });
	Object3dFactory::Register("Event_BossSpawn", [](const std::string& n) { return std::make_unique<BossSpawnEvent>(n); });
	Object3dFactory::Register("Event_Clear", [](const std::string& n) { return std::make_unique<ClearEvent>(n); });
}
