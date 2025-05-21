#pragma once
#include <Vector3.h>
#include <Quaternion.h>
#include <Matrix4x4.h>
#include <vector>
#include <map>
#include <string>
#include <optional>
#include "Model/ModelStructs.h"

//#include <Skeleton.h>

class SkinnedModel;
class Skeleton;

class Animation {
public:
	// 初期化
	void Initialize(SkinnedModel* model, const std::string& filename);
	// 再生
	void Update();
	// Skeletonを適応する
	//void ApplySkeleton();
private:
	// アニメーションを読む処理
	AnimationData LoadAnimationFile(const std::string& filename);
public:
	// 任意の時刻を取得する (Vector3)
	static Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time);
	// 任意の時刻を取得する (Quaternion)
	static Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time);
private:
	// アニメーション
	AnimationData animation_;
	// モデルデータ
	SkinnedModel* model_;
	// 現在のアニメーションタイム
	float animationTime = 0.0f;

public: // ゲッター // セッター //
	AnimationData GetAnimation() { return animation_; }
};