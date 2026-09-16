#include "TitleScene.h"
#include "Graphics/Resource/TextureManager.h"
#include <Graphics/Rendering/Sprite/SpriteManager.h>
#include <Input/Input.h>
#include <Stage/SceneLoader.h>
#include <Stage/SceneBuilder.h>
#include <World3D/Object/Model/ModelManager.h>
#include <Graphics/Rendering/Sky/SkySystem.h>
#include <World3D/Light/PointLight.h>
#include <World3D/Object/Renderer/ModelRenderer.h>
#include <World3D/Object/Renderer/RendererManager.h>
#include <Scene/Transition/FadeTransition.h>
#include <Scene/Transition/TransitionManager.h>
#include <Scene/Transition/SceneTransitionController.h>
#include <GameObject/Camera/TitleCamera.h>
#include <World3D/Collider/CollisionManager.h>
#include <Debugger/GlobalVariables.h>
#include <Utility/DeltaTime.h>
#include <Audio/SoundManager.h>
#include "Audio/GameSoundLibrary.h"
#include <GameObject/Character/Player/Player.h> // 先読みするモデル名をゲーム中と共有する
#include <GameData/GameSession.h>
#include <Graphics/Rendering/PostEffect/OffScreenManager.h>
#include <Graphics/Rendering/PostEffect/VignetteEffect.h>
#include <cmath>

namespace {
// タイトルの常時ビネット。四隅でも6割ほどの明るさに留まる「軽い」かかり方にしてある
// （中心からの距離 0.4 から暗くなり始め、0.8 で最大。画面の角は距離0.71）
constexpr const char* kTitleVignetteName = "TitleVignette";
constexpr float kTitleVignetteRadius = 0.4f;
constexpr float kTitleVignetteSoftness = -0.4f;
constexpr float kTitleVignetteIntensity = 0.45f;
}


void TitleScene::Initialize() {
	// 必ず LoadSkinnedModel で読むこと。
	// ModelManager::FindModel は静的モデルを先に探すので、ここで LoadModel してしまうと
	// 同じ名前で静的モデルが登録され、ゲーム中のプレイヤーが黙ってアニメーションしなくなる
	ModelManager::GetInstance().LoadSkinnedModel(Player::kModelName);
	ModelManager::GetInstance().LoadModel("Sword");
	ModelManager::GetInstance().LoadModel("Cube");
	TextureManager::GetInstance().LoadTexture("white.png");
	TextureManager::GetInstance().LoadTexture("Title.png");
	TextureManager::GetInstance().LoadTexture("circle.png");
	TextureManager::GetInstance().LoadTexture("circle2.png");
	TextureManager::GetInstance().LoadTexture("TitleUnder.png");
	TextureManager::GetInstance().LoadTexture("TitleUp.png");
	TextureManager::GetInstance().LoadTexture("SelectArrow.png");
	TextureManager::GetInstance().LoadTexture("smoke.png");
	// メニューの下敷きに使う絵。文字は操作案内も含めてフォントから描くので、文字のテクスチャは読まない
	TitleMenu::LoadTextures();

	// タイトルのBGM。読み込み（数十MB）もここで済ませる
	SoundManager::GetInstance().PlayBGM("TitleBGM", 1.5f);

	// カメラの生成

	cameraManager_->AddCamera(std::make_unique<TitleCamera>("TitleCamera"));
	cameraManager_->SetActiveCamera("TitleCamera");

	camera_ = static_cast<TitleCamera*>(cameraManager_->GetActiveCamera());
	camera_->Initialize();
	camera_->Enter();

	// タイトルシーンにあるもやもやを生成
	ParticleManager::GetInstance().CreateParticleGroup("TitleSphere", "circle2.png");
	ParticleManager::GetInstance().CreateEmitter("TitleSphere", "TitleSphere");
	ParticleManager::GetInstance().CreateParticleGroup("TitleSmoke", "circle.png");
	ParticleManager::GetInstance().CreateEmitter("TitleSmoke", "TitleSmoke");
	ParticleManager::GetInstance().CreateParticleGroup("TitleSmoke2", "smoke.png");
	ParticleManager::GetInstance().CreateEmitter("TitleSmoke2", "TitleSmoke2");

	// スカイボックスを生成
	SkySystem::GetInstance().CreateSkyBox("qwantani_moon_noon_puresky_4k.dds");

	lightManager_->AddLight(std::make_unique<PointLight>("TitlePoint"));
	lightManager_->AddLight(std::make_unique<SpotLight>("TitleSpot"));
	lightManager_->AddLight(std::make_unique<DirectionalLight>("TitleDir"));

	// 明滅の中心にする明るさを、jsonで設定された値から拾っておく
	basePointLightIntensity_ = GlobalVariables::GetInstance().GetValueRef<float>(kPointLightName, "Intensity");

	SceneBuilder::BuildScene(SceneLoader::Load("Resource/Stage/Title.json"));

	titleUI_ = std::make_unique<TitleUI>();
	titleUI_->Initialize();

	titleMenu_ = std::make_unique<TitleMenu>();
	titleMenu_->Initialize();

	// 新しいトランジションの追加。
	// 生成してから登録を弾かれると、コンストラクタで作った常駐スプライトだけが
	// SpriteManager に残ってしまうので、登録済みかを先に確かめる
	if (!TransitionManager::GetInstance().HasTransition("Fade")) {
		TransitionManager::GetInstance().AddTransition(std::make_unique<FadeTransition>("Fade"));
	}
	TransitionManager::GetInstance().SetTransition("Fade");

	// 起動直後はシーン遷移を経由しないので、ここで暗転明けを始める。
	// 他シーンから来た場合は遷移側が動いているため、この呼び出しは無視される
	SceneTransitionController::GetInstance().BeginFadeIn();

	// 画面の四隅を軽く暗くして、中央のロゴとメニューへ目を寄せる。
	// ポストエフェクトはシーンをまたいで残るので、2回目以降は作らずに既存のものを使う
	OffScreenManager& offScreen = OffScreenManager::GetInstance();
	if (!offScreen.FindEffect(kTitleVignetteName)) {
		auto vignette = std::make_unique<VignetteEffect>(kTitleVignetteName);
		vignette->SetColor(0.0f, 0.0f, 0.0f);
		offScreen.AddEffect(std::move(vignette));
	}
	if (auto* vignette = static_cast<VignetteEffect*>(offScreen.FindEffect(kTitleVignetteName))) {
		VignetteEffect::VignetteEffectData& data = vignette->GetEffectData();
		data.radius = kTitleVignetteRadius;
		data.intensity = kTitleVignetteIntensity;
		data.softness = kTitleVignetteSoftness;
		vignette->SetActive(true);
	}
}

void TitleScene::Finalize() {
	// タイトルのビネットはゲーム中に残さない（エフェクト自体は次にタイトルへ戻ったとき使い回す）
	if (auto* vignette = OffScreenManager::GetInstance().FindEffect(kTitleVignetteName)) {
		vignette->SetActive(false);
	}

	// 明滅させていたぶんを戻しておく。
	// ライトの明るさは json 側の値をそのまま書き換えて動かしているので、
	// 途中の値のまま抜けるとエディタで保存したときにその値が焼き付いてしまう
	if (basePointLightIntensity_ > 0.0f) {
		GlobalVariables::GetInstance().GetValueRef<float>(kPointLightName, "Intensity") = basePointLightIntensity_;
	}

	SpriteManager::GetInstance().DeleteNonPersistentSprite();
	Object3dManager::GetInstance().DeleteAllObject();
	CollisionManager::GetInstance().DeleteAllCollider();
	RendererManager::GetInstance().DeleteAllRenderer();
	CameraManager::GetInstance().DeleteAllCamera();
	LightManager::GetInstance().DeleteAllLight();
	ParticleManager::GetInstance().DeleteAllEmitters();
}

void TitleScene::Update() {
	titleUI_->Update();
	titleMenu_->Update();

	// 導入のカメラ移動が終わったら操作案内を出す
	if (camera_->IsIdle()) {
		titleUI_->ShowPrompt();
	}

	UpdateLightPulse();

	ChangePhase();
}

void TitleScene::Draw() {
	ParticleManager::GetInstance().Draw();
}

#ifdef _DEBUG
void TitleScene::DebugUpdate() {
}
#endif // _DEBUG

void TitleScene::UpdateLightPulse() {
	lightPulseTimer_ += DeltaTime::GetDeltaTime();

	// プレイヤーを照らす青いライトを息づかせる。
	// PointLight::Update() は毎フレーム GlobalVariables から値を読み直すので、
	// ライトの実体ではなくこちらの値を動かさないと反映されない
	const float pulse = 1.0f + std::sin(lightPulseTimer_ * 1.60f) * 0.10f + std::sin(lightPulseTimer_ * 4.30f) * 0.03f;

	GlobalVariables::GetInstance().GetValueRef<float>(kPointLightName, "Intensity") = basePointLightIntensity_ * pulse;
}

void TitleScene::ChangePhase() {
	// 演出が終わっている・終了処理に入っているなら入力は受け付けない
	if (camera_->IsExit() || isQuitting_) return;

	// メニューが出ていれば、そこから先の操作はメニューが受け持つ
	if (titleMenu_->IsOpen()) {
		ApplyMenuResult();
		return;
	}

	// パッドとキーボードのどちらでも進めるようにする。
	// 押しっぱなし判定だと、前のシーンで押していたボタンをそのまま拾って
	// タイトルに来た瞬間に開始してしまうので、必ず押した瞬間だけを見る
	const Input& input = Input::GetInstance();
	const bool decided = (input.IsConnected() && input.TriggerButton(PadNumber::ButtonA))
		|| input.TriggerKey(DIK_SPACE);

	if (!decided) return;

	// 導入のカメラ移動中なら、まずはそれを飛ばす（もう一度押すと開始）
	if (camera_->IsEntering()) {
		camera_->SkipEnter();
		return;
	}

	// 操作案内を消して、入れ替わりにメニューを開く
	SoundManager::GetInstance().PlaySE(GameSound::kTitlePress, 0.8f);
	titleUI_->Exit();
	titleMenu_->Open();
}

void TitleScene::ApplyMenuResult() {
	switch (titleMenu_->GetResult()) {
	case TitleMenu::Result::StartGame:
		// どちらのモードで始めるかは GameScene が GameSession から読む。
		// GAMEPLAY への切り替えは、飛び込む演出の途中で TitleCamera が要求する
		GameSession::BeginStory();
		camera_->Exit();
		titleMenu_->Close();
		break;
	case TitleMenu::Result::StartTraining:
		// 行き先のシーンは本編と同じ GAMEPLAY。中身の違いは GameScene が分岐する。
		// 相手の指定は無し（一覧の先頭で始まり、部屋の中の設定メニューで切り替える）
		GameSession::BeginTraining();
		camera_->Exit();
		titleMenu_->Close();
		break;
	case TitleMenu::Result::Quit:
		isQuitting_ = true;
		// 暗転に合わせて曲も引く。暗転しきったところでアプリが終わる
		SoundManager::GetInstance().StopBGM(0.6f);
		SceneTransitionController::GetInstance().RequestApplicationQuit();
		break;
	default:
		break;
	}
}
