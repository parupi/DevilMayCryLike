#pragma once
#include <function.h>
#include <ParticleManager.h>
class ParticleEmitter
{
public:
	// 初期化
	void Initialize(std::string name);
	// 更新
	void Update(Vector3 Position, uint32_t count);
	// 発生
	void Emit() const;
private:

	struct Emitter {
		std::string name; //!< パーティクルの名前
		EulerTransform transform; //!< エミッタのTransform
		uint32_t count; //!< 発生数
		float frequency; //!< 発生頻度
		float frequencyTime; //!< 頻度用時刻
		bool isActive;
	};

private:
	//ParticleManager* particleManager_;
	Emitter emitter{};
	float kDeltaTime = 1.0f / 60.0f;

public:
	void SetFrequency(float time) { emitter.frequency = time; }
	void SetActiveFlag(bool flag) { emitter.isActive = flag; }

};