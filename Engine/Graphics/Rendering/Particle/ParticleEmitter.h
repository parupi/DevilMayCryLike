#pragma once
#include "Math/MathUtils.h"
#include "Utility/DeltaTime.h"
#include "World3D/WorldTransform.h"

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
	void Initialize(ParticleManager* particleManager, const std::string& name, const std::string& dataName = "");
	/// <summary>更新</summary>
	/// <param name="deltaTime">VFX用のデルタタイム（ParticleManager から渡される）</param>
	void Update(float deltaTime);
	// 発生
	void Emit();

	/// <summary>
	/// 位置を指定して1回だけ発生させる（攻撃ヒット等のワンショット用）。
	/// isActive / frequency とは無関係に必ず発生する。
	/// </summary>
	/// <param name="countScale">発生数の倍率。攻撃の強さで演出量を変えるのに使う</param>
	void PlayOneShot(const Vector3& position, float countScale = 1.0f);
	/// <summary>
	/// 方向も指定して1回だけ発生させる。
	/// 方向が効くのはパーティクルグループ側で UseDirectional を有効にした場合のみ。
	/// </summary>
	void PlayOneShot(const Vector3& position, const Vector3& direction, float countScale = 1.0f);
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
	// Emit / PlayOneShot の共通実装
	void EmitAt(const Vector3& position, const Vector3* direction, float countScale);

	ParticleManager* particleManager_;
	bool emitAll_ = false;
	Emitter emitter{};
	// 発生対象のパーティクル
	std::vector<EmitterParticle> particles_;

	// メッシュ形状エミット用のモデル名（空なら通常の点エミット）
	std::string shapeModelName_;

	std::unique_ptr<WorldTransform> transform_;
public:
	void SetFrequency(float time) { emitter.frequency = time; }
	void SetActiveFlag(bool flag) { emitter.isActive = flag; }
	void SetParent(WorldTransform* transform) { transform_->SetParent(transform); }
	void SetTranslate(const Vector3& translate) { emitter.transform.translate = translate; }

	// ======================
	// メッシュ形状エミット
	// ======================

	/// <summary>
	/// エミッターの形状を ModelManager に読み込み済みのモデルにする。
	/// 設定するとモデルのメッシュ表面からパーティクルが発生する。
	/// SetParent した親のワールド行列（回転・スケール込み）で形状が追従する。
	/// </summary>
	void SetShapeModel(const std::string& modelName) { shapeModelName_ = modelName; }
	/// <summary>メッシュ形状エミットを解除して通常の点エミットに戻す</summary>
	void ClearShapeModel() { shapeModelName_.clear(); }
	const std::string& GetShapeModelName() const { return shapeModelName_; }
};