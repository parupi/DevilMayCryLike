#pragma once
#include <math/function.h>
#include <base/Particle/ParticleManager.h>
#include <base/utility/DeltaTime.h>
class ParticleEmitter
{
public:
	ParticleEmitter() = default;
	~ParticleEmitter() = default;

	// 初期化
	void Initialize(std::string name_);
	// 更新
	void Update(Vector3 position = {0.0f, 0.0f, 0.0f});
	// 発生
	void Emit() const;

	void UpdateParam() const;
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
	bool emitAll_ = false;
	Emitter emitter{};

public:
	void SetFrequency(float time) { emitter.frequency = time; }
	void SetActiveFlag(bool flag) { emitter.isActive = flag; }

};