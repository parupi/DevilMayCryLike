#pragma once
#include "Sprite.h"
#include <Object3d.h>
#include "WorldTransform.h"
#include "Particle/ParticleEmitter.h"
#include <GlobalVariables.h>

enum class Status {
	None,
	FadeIn,
	FadeOut
};

class Fade
{
public: // メンバ関数（公開）

	/// <summary>
	/// 初期化
	/// </summary>	
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>		 
	void Update();

	/// <summary>
	/// 描画
	/// </summary>		 
	void Draw();

	/// <summary>
	/// 描画
	/// </summary>		 
	void DrawSprite();

	void Start(Status status, float duration);

	void Stop();

	bool IsFinished() const;

	void SetIsGameScene(bool flag) { isGameScene_ = flag; }

private:
	void FadeOutUpdate();

	void FadeInUpdate();

private: // メンバ変数（非公開）
	GlobalVariables* global_ = GlobalVariables::GetInstance();

	std::unique_ptr<Sprite> sprite_ = nullptr;

	Status status_ = Status::None;
	float duration_ = 0.0f;
	float counter_ = 0.0f;

	//std::unique_ptr<ParticleManager> particleManager_;
	//std::unique_ptr<ParticleEmitter> emitter_;

	// パーティクル変数
	Vector3 minTranslate{};
	Vector3 maxTranslate{};
	float minScale{};
	float maxScale{};
	Vector3 minVelocity{};
	Vector3 maxVelocity{};
	float alphaCounter_ = 0.0f;
	float alpha_ = 0.0f;

	bool isGameScene_ = false;
};