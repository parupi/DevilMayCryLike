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
#include "Utility/Logger.h"
#include "Graphics/Rendering/Sky/SkySystem.h"
#include "GameObject/Event/EventManager.h"
#include "Scene/Transition/TransitionManager.h"
#include "Scene/Transition/SceneTransitionController.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include "Scene/GameScene/State/GameSceneStatePlay.h"
#include "State/GameSceneStateMenu.h"
#include "State/GameSceneStateStart.h"
#include "State/GameSceneStateClear.h"
#include "State/GameSceneStateGameOver.h"
#include "State/GameSceneStateTrainingMenu.h"
#include <GameObject/Character/Enemy/Enemy.h>
#include <GameData/GameSession.h>
#include "GameObject/Effect/HitEffectSystem.h"
#include "World3D/Object/Object3dManager.h"
#include "Input/Input.h"
#include "Audio/SoundManager.h"
#include <cmath>

void GameScene::Initialize() {
	// 戦闘中に差し替えるぶんも含めて先に読み込んでおく。
	// BGM は1曲で数十MBあり、戦闘が始まってから読むと確実に引っかかる
	SoundManager::GetInstance().Preload("GamePlayBGM");
	SoundManager::GetInstance().Preload("BattleBGM");
	// 死亡SE。resource/sound/PlayerDeath.wav を置けば鳴る（無ければ Preload/PlaySE は何もしない）
	SoundManager::GetInstance().Preload("PlayerDeath");
	SoundManager::GetInstance().PlayBGM("GamePlayBGM", 1.5f);

	// ステートの生成
	states_["Start"] = std::make_unique<GameSceneStateStart>();
	states_["Play"] = std::make_unique<GameSceneStatePlay>();
	states_["Menu"] = std::make_unique<GameSceneStateMenu>();
	states_["Clear"] = std::make_unique<GameSceneStateClear>();
	states_["GameOver"] = std::make_unique<GameSceneStateGameOver>();
	if (IsTrainingMode()) {
		// 相手の種類や無敵を切り替える設定メニュー。本編には遷移先が無いので作らない
		states_["TrainingMenu"] = std::make_unique<GameSceneStateTrainingMenu>();
	}
	// トレーニングは何度も入り直すので、開始演出(StageStart)を挟まずすぐ動ける状態から始める
	currentState_ = IsTrainingMode() ? states_["Play"].get() : states_["Start"].get();

	// 入力の受付状態を管理するクラス生成
	inputContext_ = std::make_unique<InputContext>();
	inputContext_->Initialize(&Input::GetInstance());

	// カメラの生成
	std::unique_ptr<GameCamera> camera = std::make_unique<GameCamera>("GameCamera");
	camera->GetTranslate() = {0.096f, 13.4f, -20.0f};
	camera->GetRotate() = {0.5f, -0.005f, 0.0f};
	gameCamera_ = camera.get();
	cameraManager_->AddCamera(std::move(camera));

	// カメラの生成
	std::unique_ptr<ClearCamera> clearCamera = std::make_unique<ClearCamera>("ClearCamera");
	cameraManager_->AddCamera(std::move(clearCamera));

	// 既定のカメラをここで立てておく。
	// AddCamera はアクティブにしないので、これが無いと GetCurrentCamera() が null のまま
	// Object3d::Update() まで流れてアクセス違反になる。
	// 本編は直後の StageStart が "StartCamera" へ差し替えるので見た目は変わらないが、
	// 開始演出を挟まない経路（トレーニング）はこれが唯一の設定になる
	cameraManager_->SetActiveCamera("GameCamera");

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
	// 死亡演出でディゾルブの前に立ち上る黒いもや（拡散する粒子よりゆっくり・大きい）。
	// 敵とプレイヤーで共有する
	ParticleManager::GetInstance().CreateParticleGroup("DeathSmoke", "smoke.png");
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

	// ステージの情報を読み込んで生成。
	// 本編で読むステージはエディタで切り替えられる（Release では既定のまま）が、
	// トレーニングは専用ステージで固定される
	const std::string stagePath = GameSession::GetStagePath();
	SceneBuilder::BuildScene(SceneLoader::Load(stagePath));

	lightManager_->AddLight(std::make_unique<DirectionalLight>("GameDirectionalLight"));

	lockOnSystem_ = std::make_unique<LockOnSystem>();

	// エディタでステージを作れるようになったぶん、Player を置き忘れたステージも起こりうる。
	// そのまま進むとヌル参照で落ちて原因が分からないので、ここではっきり止める
	player_ = dynamic_cast<Player*>(Object3dManager::GetInstance().FindObject("Player"));
	ASSERT_MSG(player_ != nullptr,
		("ステージに Player クラスのオブジェクトがありません。\n  ステージ: "
			+ stagePath
			+ "\n  Hierarchy の「+作成」でクラス Player を置いて保存してください。").c_str());

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
	mask_->SetSize({1280.0f, 720.0f});
	mask_->SetColor({0.0f, 0.0f, 0.0f, 0.5f});

	menuUI_ = std::make_unique<MenuUI>();
	menuUI_->Initialize(this);

	// スタイリッシュランクのゲーム中HUD（画面右・中央高さ）
	styleHud_ = std::make_unique<StyleHUD>();
	styleHud_->Initialize();

	// トレーニングの状態表示。ポーズの暗幕より先に作って、暗幕がこの上に来るようにする
	if (IsTrainingMode()) {
		trainingHud_ = std::make_unique<TrainingHUD>();
		trainingHud_->Initialize();

		// 設定メニューは状態表示より後に作る。
		// 同じレイヤーでは後から作ったものが手前に出るので、こうしないと
		// メニューの暗幕の上に状態表示が乗ってしまう
		trainingMenu_ = std::make_unique<TrainingMenu>();
		trainingMenu_->Initialize();
	}

	// 死亡時の選択肢。ゲーム中のHUDより後に作って、暗幕がHUDの上に来るようにする
	gameOverUI_ = std::make_unique<GameOverUI>();
	gameOverUI_->Initialize();

	tutorial_ = std::make_unique<TutorialSystem>();
	tutorial_->Initialize();
	// PlayerのチュートリアルサービスをGameSceneのものに接続する
	// (これが無いとPlayer側のGetTutorialService()がnullptrを返しクラッシュする)
	player_->SetTutorialService(tutorial_.get());

	if (IsTrainingMode()) {
		// トレーニングでチュートリアルは流さない。
		// TutorialDummy::CanDie() が全チュートリアル完了を条件にしているので、
		// 先に完了扱いにしておかないと倒せない相手になってしまう
		tutorial_->SkipAllTutorials();

		// 相手の生成もリセットの基準位置もここが持つ。プレイヤー生成後に初期化すること
		training_ = std::make_unique<TrainingController>();
		training_->Initialize(player_, lockOnSystem_.get());
	}

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

void GameScene::Update() {
	//lightManager_->Update();
	//gameUI_->Update();

	// トレーニングの相手の生成・出し直し。
	// Object3dManager::Update() の中ではオブジェクト配列を回している最中で追加できないが、
	// シーンの更新はその手前で走るのでここなら安全。
	// キー操作を受けるのは操作中（Play）だけ。ポーズやゲームオーバーの選択中に
	// トレーニングのキーまで効くと事故になる
	if (training_) {
		const bool inPlay = (currentState_ == states_["Play"].get());
		training_->Update(inPlay);
	}

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

	mask_->SetColor({0.0f, 0.0f, 0.0f, maskAlpha_});
	mask_->Update();

	menuUI_->Update();
	gameOverUI_->Update();

	// スタイルランクHUDは戦闘中のみ表示する（死亡演出に入ったら引っ込める）
	if (player_ && styleHud_) {
		auto* score = player_->GetScoreManager();
		if (score && score->IsBattleActive() && !player_->IsDying()) {
			styleHud_->Update(score->GetCurrentRank(), score->GetCurrentScore());
		} else {
			styleHud_->Hide();
		}
	}

	// トレーニングの状態表示。ポーズ・ゲームオーバー・設定メニュー中は邪魔になるので引っ込める
	if (trainingHud_ && training_) {
		if (currentState_ == states_["Play"].get()) {
			trainingHud_->Update(*training_, player_);
		} else {
			trainingHud_->Hide();
		}
	}

	// 設定メニュー。閉じるアニメを進めるため、開いていないフレームでも呼ぶ。
	// 相手の出し直しもここから走るが、Object3dManager::Update() の手前なので追加して構わない
	if (trainingMenu_) {
		trainingMenu_->Update(training_.get());
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

bool GameScene::IsTrainingMode() const {
	return GameSession::GetMode() == GameMode::Training;
}

void GameScene::ChangeState(const std::string& stateName) {
	currentState_->Exit(*this);
	auto it = states_.find(stateName);
	if (it != states_.end()) {
		currentState_ = it->second.get();
		currentState_->Enter(*this);
	}
}
