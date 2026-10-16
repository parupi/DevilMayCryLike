#pragma once
#include "Scene/BaseScene.h"
#include <memory>

#include <GameObject/UI/ClearUI/ClearUI.h>
#include <GameObject/UI/Common/MenuNavigator.h>
#include "Clear/ClearSequence.h"
#include "Math/Vector3.h"

class BaseCamera;
class CaveDustEmitter;
class Object3d;
class StageSpotLight;

/// <summary>
/// クリア画面（クリアシーン演出仕様書）。
///
/// ゲームと同じ洞窟の回廊（Resource/Stage/Clear.json）の中に勝ったプレイヤーを立たせ、
/// 頭上から一筋の光を当てる。回廊の奥には洞窟の出口があり、外の暖かい光が霧越しに差し込む。
/// 画面の左に結果を時間差で出し（ClearUI）、進行は ClearSequence の時刻で決まる。
/// </summary>
class ClearScene : public BaseScene
{
public:
	ClearScene() = default;
	~ClearScene() override;

	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Draw() override;

#ifdef _DEBUG
	void DebugUpdate() override;
#endif // _DEBUG

private:
	void CreatePlayerDisplay();
	void UpdateLighting();
	void UpdateCamera();
	void ApplyResult();

	std::unique_ptr<ClearUI> clearUI_;
	std::unique_ptr<CaveDustEmitter> caveDust_;
	ClearSequence sequence_;
	MenuNavigator navigator_;

	BaseCamera* camera_ = nullptr;
	float baseFov_ = 0.0f;

	// 頭上の光（ステージの Cave_ClearMainLight）。演出で少し強めるので元の値を覚えておく
	StageSpotLight* mainLight_ = nullptr;
	float mainLightIntensity_ = 0.0f;
	float mainLightVolumetric_ = 0.0f;

	Vector3 playerPosition_{};
	bool decided_ = false;

#ifdef _DEBUG
	// 各ランクの見た目を確かめるための差し替え（-1 なら実際の結果）
	int debugRankIndex_ = -1;
	int debugScore_ = 125800;
#endif
};
