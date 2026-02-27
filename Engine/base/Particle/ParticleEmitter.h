#pragma once
#include "math/function.h"
#include "base/utility/DeltaTime.h"
#include "3d/WorldTransform.h"

class ParticleManager;

struct Emitter {
	std::string name; // エミッターの名前
	EulerTransform transform; //!< エミッタのTransform
	uint32_t count; //!< 発生数
	float frequency; //!< 発生頻度
	float frequencyTime; //!< 頻度用時刻
	bool isActive;
};

struct EmitterParticle
{
	std::string name;
	int count;
	float spawnRate = 1.0f;
};


class ParticleEmitter
{
public:
	ParticleEmitter() = default;
	~ParticleEmitter() = default;

	// 初期化
	void Initialize(ParticleManager* particleManager, const std::string& name);
	// 更新
	void Update(Vector3 position = {0.0f, 0.0f, 0.0f});
	// 発生
	void Emit();
	// パーティクルを追加
	void AddParticle(const std::string& name);
	// 発生対象から削除
	void RemoveParticle(size_t index);
	// 現在発生する対象のパーティクルを取得
	std::vector<EmitterParticle>& GetParticles() { return particles_; }
	// 現在の設定を保存
	void Save(const std::string& path);
	// 保存したエミッターのファイルを読み込む
	void Load(const std::string& path);
private:
	ParticleManager* particleManager_;
	bool emitAll_ = false;
	Emitter emitter{};
	// 発生対象のパーティクル
	std::vector<EmitterParticle> particles_;

	std::unique_ptr<WorldTransform> transform_;
public:
	void SetFrequency(float time) { emitter.frequency = time; }
	void SetActiveFlag(bool flag) { emitter.isActive = flag; }
	void SetParent(WorldTransform* transform) { transform_->SetParent(transform); }


};