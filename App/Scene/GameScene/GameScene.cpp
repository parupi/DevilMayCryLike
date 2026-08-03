#include "GameScene.h"
#include "Graphics/Resource/TextureManager.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#include "Debugger/ImGuiManager.h"
#include "Math/Quaternion.h"
#include "Math/Vector3.h"
#include "Math/Matrix4x4.h"
#include "World3D/Object/Model/ModelManager.h"
#include "World3D/Object/Renderer/RendererManager.h"
#include "World3D/Object/Renderer/ModelRenderer.h"
#include "World3D/Collider/CollisionManager.h"
#include "World3D/Object/Renderer/PrimitiveRenderer.h"
#include "Stage/SceneLoader.h"
#include "Stage/SceneBuilder.h"
#include "Graphics/Rendering/Sky/SkySystem.h"
#include "GameObject/Event/EventManager.h"
#include "Scene/Transition/TransitionManager.h"
#include "Scene/Transition/SceneTransitionController.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include "Scene/GameScene/State/GameSceneStatePlay.h"
#include "State/GameSceneStateMenu.h"
#include "State/GameSceneStateStart.h"
#include "State/GameSceneStateClear.h"
#include <GameObject/Character/Enemy/Enemy.h>
#include "GameObject/Effect/HitEffectSystem.h"
#include "World3D/Object/Object3dManager.h"
#include "Input/Input.h"
#include <cmath>

void GameScene::Initialize() {
	// ステートの生成
	states_["Start"] = std::make_unique<GameSceneStateStart>();
	states_["Play"] = std::make_unique<GameSceneStatePlay>();
	states_["Menu"] = std::make_unique<GameSceneStateMenu>();
	states_["Clear"] = std::make_unique<GameSceneStateClear>();
	currentState_ = states_["Start"].get();

	// 入力の受付状態を管理するクラス生成
	inputContext_ = std::make_unique<InputContext>();
	inputContext_->Initialize(&Input::GetInstance());

	// カメラの生成
	std::unique_ptr<GameCamera> camera = std::make_unique<GameCamera>("GameCamera");
	camera->GetTranslate() = { 0.096f, 13.4f, -20.0f };
	camera->GetRotate() = { 0.5f, -0.005f, 0.0f };
	gameCamera_ = camera.get();
	cameraManager_->AddCamera(std::move(camera));

	// カメラの生成
	std::unique_ptr<ClearCamera> clearCamera = std::make_unique<ClearCamera>("ClearCamera");
	cameraManager_->AddCamera(std::move(clearCamera));

	ParticleManager::GetInstance().CreateParticleGroup("test", "circle.png");
	ParticleManager::GetInstance().CreateParticleGroup("fire", "circle.png");
	ParticleManager::GetInstance().CreateParticleGroup("smoke", "circle.png");
	ParticleManager::GetInstance().CreateParticleGroup("hitSmoke", "hitSmoke.png");
	ParticleManager::GetInstance().CreateParticleGroup("GameCircle", "circle.png");
	ParticleManager::GetInstance().CreateParticleGroup("GameSmoke", "smoke.png");
	// 旧ヒットエフェクト。ゲーム側からは "HitImpact" に置き換えたので現在は誰も発生させないが、
	// 調整済みのデータが残っているのでエディタから見えるように登録だけ残してある
	ParticleManager::GetInstance().CreateParticleGroup("EnemyDamageEffect", "white.png");
	ParticleManager::GetInstance().CreateParticleGroup("PlayerSlashEffect", "circle.png");
	ParticleManager::GetInstance().CreateParticleGroup("EnemyChargeRing", "white.png", PrimitiveType::Ring);
	// 敵の出現演出（収束する黒い粒子）・死亡演出（拡散する黒い粒子）
	ParticleManager::GetInstance().CreateParticleGroup("EnemySpawnParticle", "smoke.png");
	ParticleManager::GetInstance().CreateParticleGroup("EnemyDeathParticle", "smoke.png");
	// ボスのスーパーアーマー中（ノックバック無効）に体から立ち上る紫のオーラ
	ParticleManager::GetInstance().CreateParticleGroup("BossArmorAura", "smoke.png");
	// スーパーアーマー中の被弾で弾かれたことを示す紫の硬い火花
	ParticleManager::GetInstance().CreateParticleGroup("BossArmorHitSpark", "white.png");
	// 強制戦闘エリアの境界を示す格子状の光の壁
	ParticleManager::GetInstance().CreateParticleGroup("BattleAreaWall", "circle.png");

	// ── 攻撃ヒット時の複合VFX ──
	// 火花（ヒット方向へコーン状に飛ぶ）とインパクトリング（ヒット法線を向いて広がる）を
	// Resource/VFX/HitImpact.vfx.json が1ファイルで定義している。
	// テクスチャも形状もカーブもファイル側にあるので、VFXを差し替えてもここは変わらない。
	// 再生は HitEffectSystem::Play() 経由の PlayVFX 1回のみ
	if (!ParticleManager::GetInstance().LoadVFX("HitImpact")) {
		// 読めなかった場合はヒット時に何も出なくなる。原因は Debug Log に出る
		assert(false && "Resource/VFX/HitImpact.vfx.json の読み込みに失敗しました");
	}

	// スカイボックスを生成
	SkySystem::GetInstance().CreateSkyBox("moonless_golf_4k.dds");

	// ステージの情報を読み込んで生成
	SceneBuilder::BuildScene(SceneLoader::Load("Resource/Stage/Stage.json"));

	lightManager_->AddLight(std::make_unique<DirectionalLight>("GameDirectionalLight"));

	lockOnSystem_ = std::make_unique<LockOnSystem>();

	player_ = static_cast<Player*>(Object3dManager::GetInstance().FindObject("Player"));
	player_->SetInput(inputContext_->GetPlayerInput());
	player_->SetLockOn(lockOnSystem_.get());

	lockOnSystem_->Initialize(inputContext_->GetLockOnInput(), player_);

	// 生成された敵をロックオン対象に設定する
	for (auto* obj : Object3dManager::GetInstance().GetAllObject()) {
		if (auto* enemy = dynamic_cast<Enemy*>(obj)) {
			enemy->SetupLockOn(lockOnSystem_.get());
		}
	}

	gameCamera_->Initialize(player_, lockOnSystem_.get(), inputContext_->GetCameraInput());

	// 攻撃ヒット時の演出をまとめるクラス。カメラとプレイヤーの生成後に接続する
	HitEffectSystem::GetInstance().Initialize(gameCamera_, player_);

	//gameUI_ = std::make_unique<GameUI>();
	//gameUI_->Initialize();

	mask_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::UI, "menuMask", "white.png");
	mask_->SetSize({ 1280.0f, 720.0f });
	mask_->SetColor({ 0.0f, 0.0f, 0.0f, 0.5f });

	menuUI_ = std::make_unique<MenuUI>();
	menuUI_->Initialize(this);

	// スタイリッシュランクのゲーム中HUD（画面右・中央高さ）
	styleHud_ = std::make_unique<StyleHUD>();
	styleHud_->Initialize();

	tutorial_ = std::make_unique<TutorialSystem>();
	tutorial_->Initialize();
	// PlayerのチュートリアルサービスをGameSceneのものに接続する
	// (これが無いとPlayer側のGetTutorialService()がnullptrを返しクラッシュする)
	player_->SetTutorialService(tutorial_.get());

	// 最初のステートに入る処理
	// ※必ずプレイヤー生成(BuildScene)後に呼ぶ。StageStartがプレイヤー位置を参照してアップのカメラを作るため
	currentState_->Enter(*this);
}

void GameScene::Finalize() {
	// カメラ・プレイヤーが破棄される前に参照を切る
	HitEffectSystem::GetInstance().Finalize();

	states_.clear();
	SpriteManager::GetInstance().DeleteNonPersistentSprite();
	Object3dManager::GetInstance().DeleteAllObject();
	CollisionManager::GetInstance().DeleteAllCollider();
	RendererManager::GetInstance().DeleteAllRenderer();
	CameraManager::GetInstance().DeleteAllCamera();
	LightManager::GetInstance().DeleteAllLight();

	EventManager::GetInstance().Finalize();
}

void GameScene::Update()
{
	//lightManager_->Update();
	//gameUI_->Update();

	// スタイルスコアの暗黙戦闘判定用に、最寄りの生存敵との水平距離を供給する。
	// scoreManager->Update() は下の currentState_->Update() の中で走るため、その前に渡す。
	if (player_) {
		if (auto* score = player_->GetScoreManager()) {
			const Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();
			float nearest = 1.0e9f;
			for (auto* obj : Object3dManager::GetInstance().GetAllObject()) {
				auto* enemy = dynamic_cast<Enemy*>(obj);
				if (!enemy || !enemy->IsAlive()) continue;
				const Vector3 enemyPos = enemy->GetWorldTransform()->GetTranslation();
				const float dx = enemyPos.x - playerPos.x;
				const float dz = enemyPos.z - playerPos.z;
				const float dist = std::sqrt(dx * dx + dz * dz);
				if (dist < nearest) nearest = dist;
			}
			score->SetNearestEnemyDistance(nearest);
		}
	}

	if (currentState_) {
		currentState_->Update(*this);
	}

	mask_->SetColor({ 0.0f, 0.0f, 0.0f, maskAlpha_ });
	mask_->Update();

	menuUI_->Update();

	// スタイルランクHUDは戦闘中のみ表示する
	if (player_ && styleHud_) {
		auto* score = player_->GetScoreManager();
		if (score && score->IsBattleActive()) {
			styleHud_->Update(score->GetCurrentRank(), score->GetCurrentScore());
		} else {
			styleHud_->Hide();
		}
	}

	inputContext_->Update();

	lockOnSystem_->Update();

	tutorial_->Update();

	Object3dManager::GetInstance().SetDeltaTime(sceneDeltaTime_);
}

void GameScene::Draw() {
	// プレイヤーのスプライト描画
	if (player_) {
		player_->DrawEffect();
	}

	// 全パーティクルの描画
	ParticleManager::GetInstance().Draw();
}

#ifdef _DEBUG
void GameScene::DebugUpdate() {
	// エディタのウィンドウは App/Editor/ 側が Editor::AddWindowDrawer で登録し、
	// 対象は Object3dManager / CameraManager から自分で引く。
	// シーンがウィンドウを手で呼び出す必要はもう無い
}
#endif // _DEBUG

void GameScene::ChangeState(const std::string& stateName) {
	currentState_->Exit(*this);
	auto it = states_.find(stateName);
	if (it != states_.end()) {
		currentState_ = it->second.get();
		currentState_->Enter(*this);
	}
}