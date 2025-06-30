#pragma once
#include "3d/Object/Object3d.h"
class EnemyDamageEffect : public Object3d
{
public:
	EnemyDamageEffect(std::string objectName);
	~EnemyDamageEffect() override = default;
	// 初期
	void Initialize() override;
	void InitializeGroundRippleEffect(float time);
	void InitializeDamageRingEffect(float time);


	// 更新
	void Update() override;
	void UpdateGroundRippleEffect(const Vector3& translate);
	void UpdateDamageRingEffect(const Vector3& translate);


	// 描画
	void Draw() override;

#ifdef _DEBUG
	void DebugGui() override;
#endif // _DEBUG


private:

	struct GroundRippleData {
		bool isActive;
		TimeData effectTime;
	}groundRippleData_;

	struct DamageRingData {
		bool isActive;
		TimeData effectTime;
	}damageRingData_;

	struct CameraShake {
		bool isActive = false;
		float duration = 0.5f;     // 揺れる時間
		float time = 0.0f;         // 経過時間
		float amplitude = 0.2f;    // 揺れ幅
		float frequency = 30.0f;   // 揺れの頻度（振動の速さ）

		Vector3 originalPosition{}; // 揺れ開始前のカメラ位置
	}shake;

public:
	void InitializeCameraShake(const Vector3& currentPos, float duration = 0.5f, float amplitude = 0.2f, float frequency = 30.0f);
	void UpdateCameraShake(Camera* camera);
};

