#pragma once
#include <Object3d.h>
class PlayerAttackEffect : public Object3d
{
public:
	PlayerAttackEffect();
	~PlayerAttackEffect() override = default;
	// 初期
	void Initialize() override;
	void InitializeAttackCylinderEffect(float time);
	void InitializeTargetMarkerEffect(float time);

	// 更新
	void Update() override;
	void UpdateAttackCylinderEffect(const Vector3& translate);
	void UpdateTargetMarkerEffect(const Vector3& translate);


	// 描画
	void Draw() override;

#ifdef _DEBUG
	void DebugGui() override;
#endif // _DEBUG
private:
	enum class CylinderPhase {
		Appear,
		Hold,
		Disappear,
		Inactive
	};

	enum class MarkerPhase {
		Expand,
		Wait,
		Shrink,
		End
	};

	struct AttackCylinderData {
		bool isTrue = false;
		float rotateSpeed = 0.1f;
		TimeData timeData;
		CylinderPhase phase = CylinderPhase::Inactive;
		float scaleY = 0.0f;
		float appearDuration = 0.3f;    // 拡大にかける時間（秒）
		float holdDuration = 0.4f;      // 待機時間（秒）
		float disappearDuration = 0.3f; // 縮小にかける時間（秒）
		float elapsed = 0.0f;           // 経過時間
	} attackCylinderData_;


	struct TargetMarkerData {
		bool isTrue = false;
		float rotateSpeed = 0.025f;
		TimeData timeData;
		MarkerPhase phase = MarkerPhase::Expand;
		float expandDuration = 0.2f;
		float waitDuration = 0.3f;
		float shrinkDuration = 0.3f;
		float currentTime = 0.0f;
	} targetMarkerData_;


};

