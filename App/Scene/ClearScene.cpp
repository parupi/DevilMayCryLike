#include "ClearScene.h"
#include <World3D/Camera/BaseCamera.h>
#include <World3D/Camera/CameraManager.h>
#include <World3D/Light/LightManager.h>
#include <World3D/Object/Object3dManager.h>
#include <World3D/Object/Model/ModelManager.h>
#include <World3D/Object/Model/Animation/SkinnedInstance.h>
#include <World3D/Object/Model/Animation/AnimationPlayer.h>
#include <World3D/Collider/CollisionManager.h>
#include <World3D/Object/Renderer/ModelRenderer.h>
#include <World3D/Object/Renderer/RendererManager.h>
#include <Graphics/Rendering/Particle/ParticleManager.h>
#include <Graphics/Rendering/PostEffect/OffScreenManager.h>
#include <Graphics/Rendering/PostEffect/VignetteEffect.h>
#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include <Scene/Transition/FadeTransition.h>
#include <Scene/Transition/TransitionManager.h>
#include <Scene/Transition/SceneTransitionController.h>
#include <GameData/GameData.h>
#include <Audio/SoundManager.h>
#include "Audio/GameSoundLibrary.h"
#include <GameObject/Character/Player/Player.h> // 表示するモデル名・縮尺・クリップ名をゲーム中と共有する
#include <GameObject/Effect/CaveDustEmitter.h>
#include <GameObject/Light/StageSpotLight.h>
#include <Stage/SceneLoader.h>
#include <Stage/SceneBuilder.h>
#include "Stage/StageEnvironment.h"
#include "Utility/TimeManager.h"

#include <algorithm>
#include <cmath>

#ifdef _DEBUG
#include <imgui/imgui.h>
#include <Debugger/GlobalVariables.h>
#include <Editor/Core/EditorWindowRegistry.h>
#endif

namespace {
// クリア画面の回廊。tools/cave_props/build_corridor_stages.py で作る
constexpr const char* kStagePath = "Resource/Stage/Clear.json";
// 頭上から差す一筋の光（Main Spot Light）。ステージに置いてある
constexpr const char* kMainLightName = "Cave_ClearMainLight";

// プレイヤーの立ち位置と向き。モデルの正面は +Z なので半回転させてカメラ側（-Z）を向け、さらに少し左手前へ向ける
const Vector3 kPlayerPosition = { 0.0f, 0.0f, 0.0f };
constexpr float kPlayerYawDegree = 200.0f;
// 剣は勝ったあと床に突き立てておく（タイトルと同じ置き方）。位置はプレイヤーからのずれ
const Vector3 kSwordOffset = { 0.6f, 0.75f, -0.3f };
const Vector3 kSwordRotationDegree = { 135.0f, 90.0f, 0.0f };

// 画面の四隅をわずかに暗くする（スコアの読みやすさを優先して弱め）
constexpr const char* kVignetteName = "ClearVignette";
constexpr float kVignetteRadius = 0.45f;
constexpr float kVignetteSoftness = -0.4f;
constexpr float kVignetteIntensity = 0.35f;

#ifdef _DEBUG
constexpr const char* kRankCodes[] = { "D", "C", "B", "A", "S", "SS", "SSS" };
#endif
}

ClearScene::~ClearScene() = default;

void ClearScene::Initialize()
{
	SoundManager::GetInstance().PlayBGM("ClearBGM", 0.8f);
	for (const char* se : { GameSound::kTutorialClear, GameSound::kScoreCount, GameSound::kUIConfirm,
		GameSound::kRankReveal, GameSound::kRankUp, GameSound::kUICursor, GameSound::kTorchCrackle }) {
		SoundManager::GetInstance().PreloadSE(se);
	}

	ClearUI::LoadResources();
	// 必ず LoadSkinnedModel で読む（LoadModel だと静的モデルとして登録され、ゲーム中もアニメーションしなくなる）
	ModelManager::GetInstance().LoadSkinnedModel(Player::kModelName);
	ModelManager::GetInstance().LoadModel("Sword");

	sequence_.Initialize();
	const ClearSequence::Params& params = sequence_.GetParams();

	// 固定カメラ。プレイヤーを斜め前の低い所から見上げる
	std::unique_ptr<BaseCamera> clearCamera = std::make_unique<BaseCamera>("ClearCamera");
	camera_ = clearCamera.get();
	camera_->GetTranslate() = params.cameraPosition;
	camera_->LookAt(params.cameraTarget);
	baseFov_ = camera_->GetFovY();
	CameraManager::GetInstance().AddCamera(std::move(clearCamera));
	CameraManager::GetInstance().SetActiveCamera("ClearCamera");

	// ゲームと同じ洞窟の雰囲気（空を消し、暗さ・霧・光の筋・色調整）。演出のパスより前に並べるため組み立てより先に呼ぶ
	StageEnvironment::Apply(SceneLoader::LoadEnvironmentName(kStagePath));
	SceneBuilder::BuildScene(SceneLoader::Load(kStagePath));

	CreatePlayerDisplay();

	mainLight_ = dynamic_cast<StageSpotLight*>(Object3dManager::GetInstance().FindObject(kMainLightName));
	if (mainLight_) {
		mainLightIntensity_ = mainLight_->GetParams().intensity;
		mainLightVolumetric_ = mainLight_->GetParams().volumetric;
	}

	// 埃。頭上の光の中にも撒くので、ステージを組み立てた後に作る
	caveDust_ = std::make_unique<CaveDustEmitter>();

	clearUI_ = std::make_unique<ClearUI>();
	clearUI_->Initialize();
	ApplyResult();

	// ゲームから来たときは遷移の暗転が明けていく。直接このシーンで起動したときだけここで明けを始める
	if (!TransitionManager::GetInstance().HasTransition("Fade")) {
		TransitionManager::GetInstance().AddTransition(std::make_unique<FadeTransition>("Fade"));
	}
	TransitionManager::GetInstance().SetTransition("Fade");
	SceneTransitionController::GetInstance().BeginFadeIn();

	OffScreenManager& offScreen = OffScreenManager::GetInstance();
	if (!offScreen.FindEffect(kVignetteName)) {
		auto vignette = std::make_unique<VignetteEffect>(kVignetteName);
		vignette->SetColor(0.0f, 0.0f, 0.0f);
		offScreen.AddEffect(std::move(vignette));
	}
	if (auto* vignette = static_cast<VignetteEffect*>(offScreen.FindEffect(kVignetteName))) {
		VignetteEffect::VignetteEffectData& data = vignette->GetEffectData();
		data.radius = kVignetteRadius;
		data.intensity = kVignetteIntensity;
		data.softness = kVignetteSoftness;
		vignette->SetActive(true);
	}

	UpdateLighting();
	UpdateCamera();
}

void ClearScene::CreatePlayerDisplay()
{
	RendererManager& renderers = RendererManager::GetInstance();

	std::unique_ptr<Object3d> playerObject = std::make_unique<Object3d>("Player");
	renderers.AddRenderer(std::make_unique<ModelRenderer>("Player", Player::kModelName));
	BaseRenderer* playerRenderer = renderers.FindRender("Player");
	playerObject->AddRenderer(playerRenderer);
	playerRenderer->GetWorldTransform()->GetScale() = { Player::kModelScale, Player::kModelScale, Player::kModelScale };
	if (SkinnedInstance* instance = playerRenderer->GetSkinnedInstance()) {
		instance->GetPlayer()->Play(Player::kClipIdle, true, 0.0f);
	}
	playerObject->GetWorldTransform()->GetTranslation() = kPlayerPosition;
	playerObject->GetWorldTransform()->GetRotation() = EulerDegree({ 0.0f, kPlayerYawDegree, 0.0f });
	playerPosition_ = kPlayerPosition;
	Object3dManager::GetInstance().AddObject(std::move(playerObject));

	std::unique_ptr<Object3d> swordObject = std::make_unique<Object3d>("Sword");
	renderers.AddRenderer(std::make_unique<ModelRenderer>("Sword", "Sword"));
	swordObject->AddRenderer(renderers.FindRender("Sword"));
	swordObject->GetWorldTransform()->GetTranslation() = {
		kPlayerPosition.x + kSwordOffset.x, kPlayerPosition.y + kSwordOffset.y, kPlayerPosition.z + kSwordOffset.z };
	swordObject->GetWorldTransform()->GetScale() = { 0.5f, 0.5f, 0.5f };
	swordObject->GetWorldTransform()->GetRotation() = EulerDegree(kSwordRotationDegree);
	Object3dManager::GetInstance().AddObject(std::move(swordObject));
}

void ClearScene::ApplyResult()
{
	int32_t score = GameData::GetInstance().GetClearScore();
	std::string rank = GameData::GetInstance().GetClearRank();
#ifdef _DEBUG
	if (debugRankIndex_ >= 0) {
		score = debugScore_;
		rank = kRankCodes[debugRankIndex_];
	}
#endif
	clearUI_->SetResult(score, rank);
}

void ClearScene::Finalize()
{
	// 洞窟の暗さや空の非表示を次のシーンへ持ち越さない
	StageEnvironment::Clear();
	if (auto* vignette = OffScreenManager::GetInstance().FindEffect(kVignetteName)) {
		vignette->SetActive(false);
	}

	SpriteManager::GetInstance().DeleteNonPersistentSprite();
	Object3dManager::GetInstance().DeleteAllObject();
	CollisionManager::GetInstance().DeleteAllCollider();
	RendererManager::GetInstance().DeleteAllRenderer();
	CameraManager::GetInstance().DeleteAllCamera();
	LightManager::GetInstance().DeleteAllLight();
	ParticleManager::GetInstance().DeleteAllEmitters();
}

void ClearScene::Update()
{
	// ステージのオブジェクト（松明など）へ渡す経過時間。これが無いと松明が炎を出さない
	Object3dManager::GetInstance().SetDeltaTime(TimeManager::GetGameDelta());
	StageEnvironment::Update();

	const float uiDelta = TimeManager::GetUIDelta();
	sequence_.Update(uiDelta);
	UpdateLighting();
	UpdateCamera();

	if (caveDust_ && camera_) {
		caveDust_->Update(TimeManager::GetVFXDelta(), playerPosition_, camera_->GetTranslate());
	}

	navigator_.Update();
	if (!decided_) {
		if (!sequence_.IsMenuReady()) {
			// 繰り返し遊ぶ人のために、演出中の決定でメニューまで飛ばす
			if (navigator_.IsDecide()) {
				sequence_.SkipToMenu();
			}
		} else {
			switch (clearUI_->UpdateMenu(navigator_)) {
			case ClearUI::MenuResult::Retry:
				// 遊んでいたステージ（本編ならボス戦の部屋）をもう一度。行き先は GameSession が覚えている
				decided_ = true;
				SceneTransitionController::GetInstance().RequestSceneChange("GAMEPLAY", true);
				break;
			case ClearUI::MenuResult::Title:
				decided_ = true;
				SceneTransitionController::GetInstance().RequestSceneChange("TITLE", true);
				break;
			default:
				break;
			}
		}
	}

	clearUI_->Update(sequence_, uiDelta);
}

void ClearScene::UpdateLighting()
{
	if (!mainLight_) {
		return;
	}
	// 1.0〜1.5 秒で頭上の光を少し強め、そのまま保つ
	const ClearSequence::Params& p = sequence_.GetParams();
	const float t = sequence_.Progress(p.lightBoostStart, (std::max)(p.lightBoostEnd - p.lightBoostStart, 0.0f));
	const float ease = t * t * (3.0f - 2.0f * t);
	const float scale = 1.0f + (p.lightBoostScale - 1.0f) * ease;
	StageSpotLight::Params& light = mainLight_->GetParamsRef();
	light.intensity = mainLightIntensity_ * scale;
	light.volumetric = mainLightVolumetric_ * scale;
	mainLight_->ApplyLightParams();
}

void ClearScene::UpdateCamera()
{
	if (!camera_) {
		return;
	}
	const ClearSequence::Params& p = sequence_.GetParams();
	camera_->GetTranslate() = p.cameraPosition;
	camera_->LookAt(p.cameraTarget);

	// メニューが出るまでにゆっくり寄り、ランクが出た瞬間だけ一瞬さらに寄る
	const float approach = sequence_.Progress(0.0f, p.menuStart);
	const float slow = approach * approach * (3.0f - 2.0f * approach);
	const float sinceRank = sequence_.GetTime() - p.rankEffectStart;
	const float punch = (!sequence_.WasSkipped() && sinceRank >= 0.0f) ? std::exp(-sinceRank * 6.0f) : 0.0f;
	camera_->SetFovY(baseFov_ * (1.0f - p.cameraZoom * slow - p.rankZoomPunch * punch));
}

void ClearScene::Draw()
{
	ParticleManager::GetInstance().Draw();
}

#ifdef _DEBUG
void ClearScene::DebugUpdate()
{
	// Window > Game から開く。ゲーム画面に重ならないよう、最初は閉じておく
	if (!EditorWindow::Begin("Clear Scene", EditorWindow::Category::kGame, 0, false)) {
		return;
	}
	ImGui::Text("時刻 %.2f 秒", sequence_.GetTime());
	if (ImGui::Button("最初から再生")) {
		sequence_.Restart();
		clearUI_->ResetEffects();
	}
	ImGui::SameLine();
	if (ImGui::Button("メニューまで飛ばす")) {
		sequence_.SkipToMenu();
	}

	// 各ランクの見た目を確かめる
	const char* preview = debugRankIndex_ < 0 ? "実際の結果" : kRankCodes[debugRankIndex_];
	if (ImGui::BeginCombo("ランク", preview)) {
		if (ImGui::Selectable("実際の結果", debugRankIndex_ < 0)) {
			debugRankIndex_ = -1;
			ApplyResult();
			sequence_.Restart();
		}
		for (int i = 0; i < static_cast<int>(std::size(kRankCodes)); ++i) {
			if (ImGui::Selectable(kRankCodes[i], debugRankIndex_ == i)) {
				debugRankIndex_ = i;
				ApplyResult();
				sequence_.Restart();
			}
		}
		ImGui::EndCombo();
	}
	if (ImGui::DragInt("スコア（差し替え時）", &debugScore_, 100.0f, 0, 9999999)) {
		ApplyResult();
	}

	// タイムラインとカメラ。GlobalVariables の値を直接いじる（ClearSequence が毎フレーム読み直す）
	GlobalVariables& gv = GlobalVariables::GetInstance();
	auto dragFloat = [&](const char* label, const char* key, float speed, float min, float max) {
		if (gv.HasItem(ClearSequence::kGroup, key)) {
			ImGui::DragFloat(label, &gv.GetValueRef<float>(ClearSequence::kGroup, key), speed, min, max, "%.2f");
		}
	};
	if (ImGui::CollapsingHeader("タイムライン[秒]", ImGuiTreeNodeFlags_DefaultOpen)) {
		dragFloat("光を強める 開始", "LightBoostStart", 0.01f, 0.0f, 10.0f);
		dragFloat("光を強める 終了", "LightBoostEnd", 0.01f, 0.0f, 10.0f);
		dragFloat("光の倍率", "LightBoostScale", 0.01f, 1.0f, 4.0f);
		dragFloat("STAGE CLEAR 開始", "StageClearStart", 0.01f, 0.0f, 10.0f);
		dragFloat("STAGE CLEAR 長さ", "StageClearDuration", 0.01f, 0.0f, 5.0f);
		dragFloat("スコア 開始", "ScoreStart", 0.01f, 0.0f, 10.0f);
		dragFloat("スコア 長さ", "ScoreDuration", 0.01f, 0.0f, 5.0f);
		dragFloat("ランク 開始", "RankStart", 0.01f, 0.0f, 10.0f);
		dragFloat("ランク 長さ", "RankDuration", 0.01f, 0.0f, 5.0f);
		dragFloat("ランク演出 開始", "RankEffectStart", 0.01f, 0.0f, 10.0f);
		dragFloat("メニュー 開始", "MenuStart", 0.01f, 0.0f, 12.0f);
		dragFloat("メニュー 長さ", "MenuDuration", 0.01f, 0.0f, 5.0f);
	}
	if (ImGui::CollapsingHeader("カメラ")) {
		if (gv.HasItem(ClearSequence::kGroup, "CameraPosition")) {
			ImGui::DragFloat3("位置", &gv.GetValueRef<Vector3>(ClearSequence::kGroup, "CameraPosition").x, 0.02f);
			ImGui::DragFloat3("注視点", &gv.GetValueRef<Vector3>(ClearSequence::kGroup, "CameraTarget").x, 0.02f);
		}
		dragFloat("ゆっくり寄る量", "CameraZoom", 0.002f, 0.0f, 0.3f);
		dragFloat("ランクで寄る量", "RankZoomPunch", 0.002f, 0.0f, 0.2f);
	}
	if (ImGui::Button("保存")) {
		sequence_.Save();
	}
	ImGui::TextDisabled("光・霧は Environment_Clear（GlobalVariables）、ステージは Clear.json");
	EditorWindow::End();
}
#endif // DEBUG
