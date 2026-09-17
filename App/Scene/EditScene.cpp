#include "EditScene.h"
#include <GameObject/Ground/Ground.h>
#include <World3D/Object/Model/ModelManager.h>
#include <GameObject/Camera/GameCamera.h>
#include <GameObject/Character/Player/Player.h>
#include <World3D/Collider/CollisionManager.h>
#include <Graphics/Rendering/Sky/SkySystem.h>
#include <World3D/Light/LightManager.h>
#include "World3D/Object/Object3dManager.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#include <Input/Input.h>
#include <World3D/Object/Renderer/RendererManager.h>
#include <World3D/Object/Renderer/ModelRenderer.h>
#include <World3D/Object/Model/SkinnedModel.h>
#include <World3D/Object/Model/Animation/AnimationClipSet.h>
#include <Utility/TimeManager.h>
#include <algorithm>

void EditScene::Initialize() {
	// カメラの生成
	std::unique_ptr<GameCamera> gameCamera = std::make_unique<GameCamera>("GameCamera");
	gameCamera->GetTranslate() = {0.096f, 13.4f, -20.0f};
	gameCamera->GetRotate() = {0.5f, -0.005f, 0.0f};
	CameraManager::GetInstance().AddCamera(std::move(gameCamera));
	CameraManager::GetInstance().SetActiveCamera("GameCamera");

	std::unique_ptr<BaseCamera> normalCamera = std::make_unique<BaseCamera>("NormalCamera");
	normalCamera->GetTranslate() = {0.0f, 0.0f, -10.0f};
	normalCamera->GetRotate() = {0.0f, -0.0f, 0.0f};
	CameraManager::GetInstance().AddCamera(std::move(normalCamera));
	CameraManager::GetInstance().SetActiveCamera("NormalCamera");

	// 入力の受付状態を管理するクラス生成
	inputContext_ = std::make_unique<InputContext>();
	inputContext_->Initialize(&Input::GetInstance());

	// スキニングの動作確認用。Warriorは5メッシュ／14クリップあるので複数メッシュのスキンクラスタも通る
	ModelManager::GetInstance().LoadSkinnedModel("Warrior");
	// （PlayerBody / PlayerHead / PlayerLeftArm / PlayerRightArm はモデルごと削除されたので先読みも削除。
	//   このシーンでは使っておらず、Player は自分で Player::kModelName を読む）
	ModelManager::GetInstance().LoadModel("weapon");
	ModelManager::GetInstance().LoadModel("Cube");
	ModelManager::GetInstance().LoadModel("suzannu");
	ModelManager::GetInstance().LoadModel("Sword");
	TextureManager::GetInstance().LoadTexture("uvChecker.png");
	TextureManager::GetInstance().LoadTexture("gradationLine.png");
	TextureManager::GetInstance().LoadTexture("Terrain.png");
	TextureManager::GetInstance().LoadTexture("gradationLine_brightened.png");
	TextureManager::GetInstance().LoadTexture("MagicEffect.png");
	TextureManager::GetInstance().LoadTexture("portal.png");
	TextureManager::GetInstance().LoadTexture("DeathText.png");
	TextureManager::GetInstance().LoadTexture("GameUI.png");
	TextureManager::GetInstance().LoadTexture("reticle.png");
	TextureManager::GetInstance().LoadTexture("hitSmoke.png");
	TextureManager::GetInstance().LoadTexture("UI/Arrow.png");
	TextureManager::GetInstance().LoadTexture("UI/plus.png");
	TextureManager::GetInstance().LoadTexture("UI/RBButton.png");
	TextureManager::GetInstance().LoadTexture("UI/SticDown.png");
	TextureManager::GetInstance().LoadTexture("UI/YButton.png");
	TextureManager::GetInstance().LoadTexture("white.png");

	// スカイボックスを生成
	SkySystem::GetInstance().CreateSkyBox("moonless_golf_4k.dds");

	std::unique_ptr<AABBCollider> collider = std::make_unique<AABBCollider>("Ground");
	collider->GetColliderData().isActive = true;
	collider->GetColliderData().offsetMax = {20.0f, 2.0f, 20.0f};
	collider->GetColliderData().offsetMin = {-20.0f, -2.0f, -20.0f};
	CollisionManager::GetInstance().AddCollider(std::move(collider));

	std::unique_ptr<AABBCollider> playerCollider = std::make_unique<AABBCollider>("Player");
	CollisionManager::GetInstance().AddCollider(std::move(playerCollider));

	std::unique_ptr<Ground> ground = std::make_unique<Ground>("Ground");
	ground->AddCollider(CollisionManager::GetInstance().FindCollider("Ground"));
	ground->Initialize();
	ground->GetWorldTransform()->GetScale() = {20.0f, 2.0f, 20.0f};
	ground->GetWorldTransform()->GetTranslation().y = -10.0f;
	Object3dManager::GetInstance().AddObject(std::move(ground));

	InitializeSkinTest();

	std::unique_ptr<Player> player = std::make_unique<Player>("Player");
	player->AddCollider(CollisionManager::GetInstance().FindCollider("Player"));
	player->Initialize();
	player->SetInput(inputContext_->GetPlayerInput());
	Object3dManager::GetInstance().AddObject(std::move(player));
}

void EditScene::Finalize() {
}

void EditScene::Update() {
	LightManager::GetInstance().Update();
	UpdateSkinTest();
}

// --- スキンモデルの動作確認 ---
// 同じ Warrior から2体作り、Phase2の分離とPhase3の4機能をまとめて動かす。
//   A: 走りながら上半身だけ斬る（レイヤー）＋ 手のボーンに剣を追従
//   B: ルートモーションで前進 ＋ アニメーションイベントで発光
// Deferredのまま（既定）で置いているので、GBufferと影に出ることも同時に確認できる
void EditScene::InitializeSkinTest() {
	auto* warrior = dynamic_cast<SkinnedModel*>(ModelManager::GetInstance().FindModel("Warrior"));
	if (!warrior) return;

	// 転がりの接地タイミングにイベントを置く。
	// 本来は Resource/Models/Warrior/Warrior.anim.json に書けるが、ここではコードから登録している
	// .anim.json から既に読めていれば触らない。両方から足すと二重に発火する
	AnimationClipSet* clips = warrior->GetClipSetMutable();
	auto addDefaultEvents = [clips](const char* clipName, float first, float second, const char* tag) {
		const std::vector<AnimationEvent>* existing = clips->GetEventsMutable(clipName);
		if (!existing || !existing->empty()) return;
		clips->AddEvent(clipName, first, tag);
		clips->AddEvent(clipName, second, tag);
	};
	addDefaultEvents("Roll", 0.15f, 0.60f, "impact");
	addDefaultEvents("Run", 0.10f, 0.45f, "footstep");

	struct SkinTestSetup {
		const char* name;
		float x;
	};
	const SkinTestSetup kSetups[] = {
		{"SkinTestA", -3.5f},
		{"SkinTestB", 3.5f},
	};

	BaseRenderer* renderers[2] = {};
	Object3d* objects[2] = {};

	for (int i = 0; i < 2; ++i) {
		RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>(kSetups[i].name, "Warrior"));
		renderers[i] = RendererManager::GetInstance().FindRender(kSetups[i].name);

		std::unique_ptr<Object3d> object = std::make_unique<Object3d>(kSetups[i].name);
		object->AddRenderer(renderers[i]);
		object->GetWorldTransform()->GetTranslation() = {kSetups[i].x, -4.0f, 8.0f};
		objects[i] = object.get();
		Object3dManager::GetInstance().AddObject(std::move(object));
	}

	skinRendererA_ = renderers[0];
	skinRendererB_ = renderers[1];
	skinObjectB_ = objects[1];

	// A: 下半身は走り、上半身(Torso以下)だけ斬撃を重ねる
	if (auto* instance = skinRendererA_->GetSkinnedInstance()) {
		AnimationPlayer* player = instance->GetPlayer();
		player->Play("Run", true, 0.0f);
		player->PlayLayer("Sword_Attack", "Torso", true, 0.2f);
	}

	// B: ルートモーションを抜き取って、その分だけオブジェクトを前へ動かす。
	// このモデルの Run/Walk は完全にインプレース（Root にも Hips にも移動キーが無い）ので、
	// 実際に前進が入っている Roll を使う。抜き取る先も移動を持つ Body になる
	if (auto* instance = skinRendererB_->GetSkinnedInstance()) {
		AnimationPlayer* player = instance->GetPlayer();
		player->Play("Roll", true, 0.0f);
		player->SetRootMotionJoint("Body");
	}

	// A の右手（Weapon.R）に剣を追従させる。
	// 追従側の親はオブジェクトのトランスフォームではなく「レンダラーのトランスフォーム」であること
	RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>("AttachedSword", "Sword"));
	BaseRenderer* swordRenderer = RendererManager::GetInstance().FindRender("AttachedSword");

	std::unique_ptr<Object3d> sword = std::make_unique<Object3d>("AttachedSword");
	sword->AddRenderer(swordRenderer);
	sword->GetWorldTransform()->SetParent(skinRendererA_->GetWorldTransform());
	attachedSword_ = sword.get();
	Object3dManager::GetInstance().AddObject(std::move(sword));

	swordAttachment_ = std::make_unique<BoneAttachment>();
	swordAttachment_->Initialize(skinRendererA_, "Weapon.R");
}

void EditScene::UpdateSkinTest() {
	const float deltaTime = TimeManager::GetGameDelta();

	// ボーン追従。シーン更新はオブジェクト更新より前に走るので、ここで入れる行列は
	// 1フレーム前のポーズになる。実際のゲームでは武器自身の Update（＝親の更新後）で呼ぶこと
	if (swordAttachment_ && attachedSword_) {
		swordAttachment_->Apply(attachedSword_->GetWorldTransform());
	}

	if (!skinRendererB_ || !skinObjectB_) return;

	auto* instance = skinRendererB_->GetSkinnedInstance();
	if (!instance) return;
	AnimationPlayer* player = instance->GetPlayer();

	// ルートモーションぶんだけ実際に前進させる。
	// 一定以上進んだら戻して、画面から出ていかないようにしている
	const Vector3 rootDelta = player->ConsumeRootMotion();
	Vector3& translation = skinObjectB_->GetWorldTransform()->GetTranslation();
	translation += rootDelta;
	// 進む向きはクリップ次第なので、前後どちらへ抜けても戻す
	if (translation.z > 22.0f) translation.z = 6.0f;
	if (translation.z < 6.0f) translation.z = 22.0f;

	// イベントが来たフレームで光らせる。当たり判定やSEも本来はここで出す
	if (player->WasEventFired("impact")) {
		eventFlash_ = 1.0f;
	}
	eventFlash_ = (std::max)(0.0f, eventFlash_ - deltaTime * 6.0f);
	skinRendererB_->SetEmissiveTint({0.2f, 0.6f, 1.0f, eventFlash_ * 3.0f});
}

void EditScene::Draw() {
}

#ifdef _DEBUG
void EditScene::DebugUpdate() {
	// エディタのウィンドウは App/Editor/ 側が Editor::AddWindowDrawer で登録している。
	// オブジェクトの中身は Hierarchy / Inspector から触れるので、ここでは何もしない
}
#endif // DEBUG
