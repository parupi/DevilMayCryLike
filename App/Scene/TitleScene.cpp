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
#include "Utility/TimeManager.h"
#include "World3D/Object/Object3dManager.h"
#include "Stage/StageEnvironment.h"

namespace {
// タイトルの常時ビネット。四隅でも6割ほどの明るさに留まる「軽い」かかり方にしてある
// （中心からの距離 0.4 から暗くなり始め、0.8 で最大。画面の角は距離0.71）
constexpr const char* kTitleVignetteName = "TitleVignette";
constexpr float kTitleVignetteRadius = 0.4f;
constexpr float kTitleVignetteSoftness = -0.4f;
constexpr float kTitleVignetteIntensity = 0.45f;
// タイトルの回廊。tools/cave_props/build_corridor_stages.py で作る
constexpr const char* kStagePath = "Resource/Stage/Title.json";
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
	// 壁の松明のパチパチ。鳴らす瞬間に読み込むと最初の1回だけ引っかかるので先に読む
	SoundManager::GetInstance().PreloadSE(GameSound::kTorchCrackle);

	// カメラの生成

	cameraManager_->AddCamera(std::make_unique<TitleCamera>("TitleCamera"));
	cameraManager_->SetActiveCamera("TitleCamera");

	camera_ = static_cast<TitleCamera*>(cameraManager_->GetActiveCamera());
	camera_->Initialize();
	camera_->Enter();

	// 空気のもやもやは、洞窟の雰囲気（霧の層・地面の霧）と埃（CaveDustEmitter）が受け持つ。
	// 以前の TitleSphere / TitleSmoke / TitleSmoke2 は洞窟の霧と重なって画面全体が茶色く霞むので使わない

	// スカイボックスを生成
	SkySystem::GetInstance().CreateSkyBox("qwantani_moon_noon_puresky_4k.dds");

	lightManager_->AddLight(std::make_unique<PointLight>("TitlePoint"));
	lightManager_->AddLight(std::make_unique<SpotLight>("TitleSpot"));
	lightManager_->AddLight(std::make_unique<DirectionalLight>("TitleDir"));

	// 明滅の中心にする明るさを、jsonで設定された値から拾っておく
	basePointLightIntensity_ = GlobalVariables::GetInstance().GetValueRef<float>(kPointLightName, "Intensity");

	// ゲームと同じ洞窟の雰囲気にする（空を消し、暗さ・霧・光の筋・色調整）。
	// 雰囲気の名前はステージデータの "environment"（タイトルは奥を闇に消す "Title"）。
	// 色調整のパスを組み立て中に足される演出より前に並べるため、組み立てより先に呼ぶ
	StageEnvironment::Apply(SceneLoader::LoadEnvironmentName(kStagePath));

	SceneBuilder::BuildScene(SceneLoader::Load(kStagePath));

	titleUI_ = std::make_unique<TitleUI>();
	titleUI_->Initialize();

	titleMenu_ = std::make_unique<TitleMenu>();
	titleMenu_->Initialize();

	// 埃。天井の光（ステージの SpotLight）を見て、その光の中にも撒くので、ステージを組み立てた後に作る
	caveDust_ = std::make_unique<CaveDustEmitter>();

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
	// 洞窟の暗さや空の非表示を次のシーンへ持ち越さない（ゲームシーンは自分のステージの雰囲気を掛け直す）
	StageEnvironment::Clear();

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
	// ステージのオブジェクト（松明など）へ渡す経過時間。設定するのは各シーンの役目で、
	// これが無いと 0 のまま（以前の値のまま）になり、松明が炎を出さず揺らぎも止まる
	Object3dManager::GetInstance().SetDeltaTime(TimeManager::GetGameDelta());
	StageEnvironment::Update();
	if (caveDust_ && camera_) {
		const Matrix4x4& cameraWorld = camera_->GetWorldMatrix();
		const Vector3 cameraPosition{ cameraWorld.m[3][0], cameraWorld.m[3][1], cameraWorld.m[3][2] };
		caveDust_->Update(TimeManager::GetVFXDelta(), titleUI_->GetPlayerBasePosition(), cameraPosition);
	}

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
