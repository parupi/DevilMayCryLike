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
/// クリア画面
/// </summary>
class ClearScene : public BaseScene {
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
