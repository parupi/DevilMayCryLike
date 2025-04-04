#include "Animator.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <cassert>
//#include <Model.h>
#include <Skeleton.h>

void Animator::Initialize(Model* model, const std::string& filename)
{
	model_ = model;
	LoadAnimationFile(model_->GetDirectoryPath(), filename);

	skeleton_ = std::make_unique<Skeleton>();
	skeleton_->Initialize(model_);
}

void Animator::Update()
{
	animationTime += 1.0f / 60.0f;
	animationTime = std::fmod(animationTime, animation.duration); // 最後までいったらリピート再生
	
	NodeAnimation& rootNodeAnimation = animation.nodeAnimations[model_->GetModelData().rootNode.name]; // ルートノードに入ってるアニメーションを取得
	//NodeAnimation& rootNodeAnimation = animation.nodeAnimations["mixamorig:LeftArm"]; // ルートノードに入ってるアニメーションを取得
	Vector3 scale, translate;
	Quaternion rotate;
	//if (!model_->GetModelData().isHasBones) {
		translate = CalculateValue(rootNodeAnimation.translate.keyframes, animationTime);
		rotate = CalculateValue(rootNodeAnimation.rotate.keyframes, animationTime);
		scale = CalculateValue(rootNodeAnimation.scale.keyframes, animationTime);
	//}
	//else {
	//	translate = rootNodeAnimation.translate.keyframes.empty() ? Vector3(0, 0, 0) : CalculateValue(rootNodeAnimation.translate.keyframes, animationTime);
	//	rotate = rootNodeAnimation.rotate.keyframes.empty() ? Identity() : CalculateValue(rootNodeAnimation.rotate.keyframes, animationTime);
	//	scale = rootNodeAnimation.scale.keyframes.empty() ? Vector3(1, 1, 1) : CalculateValue(rootNodeAnimation.scale.keyframes, animationTime);
	//}

	//translate = CalculateValue(rootNodeAnimation.translate.keyframes, animationTime);
	//rotate = CalculateValue(rootNodeAnimation.rotate.keyframes, animationTime);
	//scale = CalculateValue(rootNodeAnimation.scale.keyframes, animationTime);

	localMatrix = MakeAffineMatrix(scale, rotate, translate);

	// 骨ごとの状態更新
	//ApplySkeleton();

	// Skeletonの更新（状態を確定）
	if (skeleton_) {
		skeleton_->ApplyAnimation(&animation, animationTime);
		skeleton_->Update();
	}
}

Animator::Animation Animator::LoadAnimationFile(const std::string& directoryPath, const std::string& filename)
{
	//animation; // 今回作るアニメーション
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);
	assert(scene->mNumAnimations != 0); // アニメーションがない場合
	aiAnimation* animationAssimp = scene->mAnimations[0]; // 最初のアニメーションだけ採用
	animation.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond); // 時間の単位を変換

	for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {
		aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
		NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];

		// Position
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumPositionKeys; ++keyIndex) {
			aiVectorKey& keyAssimp = nodeAnimationAssimp->mPositionKeys[keyIndex];
			KeyframeVector3 keyframe{};
			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond); // 秒に変換
			keyframe.value = { -keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z }; // 右手->左手
			nodeAnimation.translate.keyframes.push_back(keyframe);
		}

		// Scale
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumScalingKeys; ++keyIndex) {
			aiVectorKey& keyAssimp = nodeAnimationAssimp->mScalingKeys[keyIndex];
			KeyframeVector3 keyframe{};
			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond); // 秒に変換
			keyframe.value = { keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z }; // 右手->左手変換は不要
			nodeAnimation.scale.keyframes.push_back(keyframe);
		}

		// Rotation
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumRotationKeys; ++keyIndex) {
			aiQuatKey& keyAssimp = nodeAnimationAssimp->mRotationKeys[keyIndex];
			KeyframeQuaternion keyframe;
			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond); // 秒に変換

			// 右手->左手座標系の変換
			keyframe.value = { -keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z, -keyAssimp.mValue.w };
			nodeAnimation.rotate.keyframes.push_back(keyframe);
		}
	}
	// 解析完了
	return animation;
}

Vector3 Animator::CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time)
{
	assert(!keyframes.empty());// キーがないものは返す値がわからないのでだめ
	if (keyframes.size() == 1 || time <= keyframes[0].time) {// キーが一つか、時刻がキーフレーム前なら最初の値とする
		return keyframes[0].value;
	}
	for (size_t index = 0; index < keyframes.size() - 1; ++index) {
		size_t nextIndex = index + 1;
		// indexとnextIndexの二つのkeyframeを取得して範囲内に時刻があるかを判定
		if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
			// 範囲内を補間する
			float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);
			return Lerp(keyframes[index].value, keyframes[nextIndex].value, t);
		}
	}
	// ここまで来た場合は一番後の時刻よりも後ろなので最後の値を返すことにする
	return (*keyframes.rbegin()).value;
}

Quaternion Animator::CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time)
{
	assert(!keyframes.empty());// キーがないものは返す値がわからないのでだめ
	if (keyframes.size() == 1 || time <= keyframes[0].time) {// キーが一つか、時刻がキーフレーム前なら最初の値とする
		return keyframes[0].value;
	}
	for (size_t index = 0; index < keyframes.size() - 1; ++index) {
		size_t nextIndex = index + 1;
		// indexとnextIndexの二つのkeyframeを取得して範囲内に時刻があるかを判定
		if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
			// 範囲内を補間する
			float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);
			return Slerp(keyframes[index].value, keyframes[nextIndex].value, t);
		}
	}
	// ここまで来た場合は一番後の時刻よりも後ろなので最後の値を返すことにする
	return (*keyframes.rbegin()).value;
}