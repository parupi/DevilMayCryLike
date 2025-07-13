#pragma once
#include <math/Vector3.h>
#include <math/Quaternion.h>
#include <math/Matrix4x4.h>
#include <vector>
#include <map>
#include <string>
#include <optional>
#include "3d/Object/Model/ModelStructs.h"

//#include <Skeleton.h>

class SkinnedModel;
class Skeleton;

class Animation {
public:
	// 初期化
	void Initialize(SkinnedModel* model, const std::string& filename);
	// 更新
	void Update();
	// 再生
	void Play(const std::string& animationName, bool isLoop, float blendTime);

	// 現在再生中のアニメーション名を取得
	const std::string& GetCurrentAnimationName() const { return currentAnimationName_; }

	// アニメーションが終了したか判定
	bool IsAnimationFinished(const std::string& animationName) const;

	// ループ設定
	void SetLoop(bool loop) { loop_ = loop; };
	bool IsLoop() const { return loop_; }

	void BlendTo(const std::string& animationName, float blendDuration, bool isLoop);

	// 現在のアニメーションデータを取得
	AnimationData* GetCurrentAnimation() { return currentAnimation_; }

	// 現在のアニメーション時間を取得
	float GetAnimationTime() const { return animationTime; }
private:

	void LoadAnimations(const std::string& filename);
public:
	// 任意の時刻を取得する (Vector3)
	static Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time);
	// 任意の時刻を取得する (Quaternion)
	static Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time);
private:
	// アニメーション
	std::map<std::string, AnimationData> animations_;
	AnimationData* currentAnimation_ = nullptr;
	// モデルデータ
	SkinnedModel* model_;
	// 現在のアニメーションタイム
	float animationTime = 0.0f;
	// ループするかどうか
	bool loop_ = false; 
	// 現在のアニメーション名
	std::string currentAnimationName_; 

	AnimationData* prevAnimation_ = nullptr;
	float blendTime_ = 0.0f;
	float blendTimer_ = 0.0f;
	bool blending_ = false;
};