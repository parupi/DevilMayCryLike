#include "Animation.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <cassert>

#include <3d/Object/Model/Animation/Skeleton.h>
#include "3d/Object/Model/SkinnedModel.h"

void Animation::Initialize(SkinnedModel* model, const std::string& filename)
{
	model_ = model;
	//animation_ = LoadAnimationFile(filename);
	LoadAnimations(filename);
}

void Animation::Update()
{
	if (!currentAnimation_) return;

	animationTime += 1.0f / 60.0f;

	if (loop_) {
		animationTime = std::fmod(animationTime, currentAnimation_->duration);
	} else {
		animationTime = std::min(animationTime, currentAnimation_->duration);
	}

	// ブレンドの進行
	if (prevAnimation_ && blendTimer_ < blendTime_) {
		blendTimer_ += 1.0f / 60.0f;
		if (blendTimer_ >= blendTime_) {
			prevAnimation_ = nullptr;
		}
	}

	// Skeleton側で全ボーンに対して適用（ブレンドも含む）
	model_->GetSkeleton()->ApplyAnimation(currentAnimation_, prevAnimation_, animationTime, blendTime_, blendTimer_);
}


void Animation::Play(const std::string& animationName, bool isLoop, float blendTime)
{
	auto it = animations_.find(animationName);
	if (it != animations_.end()) {
		// 既に再生中なら無視
		if (currentAnimationName_ == animationName) return;

		// ブレンドの準備
		prevAnimation_ = currentAnimation_;
		currentAnimation_ = &it->second;
		currentAnimationName_ = animationName;
		animationTime = 0.0f;
		loop_ = isLoop;

		// ブレンドの初期化
		blendTime_ = blendTime;
		blendTimer_ = 0.0f;
	}
}


bool Animation::IsAnimationFinished(const std::string& animationName) const
{
	auto it = animations_.find(animationName);
	if (it == animations_.end()) return true; // アニメーションが見つからない場合は終了扱い

	const AnimationData& animData = it->second;
	// ループの場合は常に終了しない
	if (loop_) return false;
	// animationTimeがduration以上なら終了
	return animationTime >= animData.duration;
}

void Animation::BlendTo(const std::string& animationName, float blendDuration, bool isLoop)
{
	auto it = animations_.find(animationName);
	if (it != animations_.end() && &it->second != currentAnimation_) {
		prevAnimation_ = currentAnimation_;
		currentAnimation_ = &it->second;
		currentAnimationName_ = animationName;
		loop_ = isLoop;

		blendTime_ = blendDuration;
		blendTimer_ = 0.0f;
		blending_ = true;
	}
}

void Animation::LoadAnimations(const std::string& filename)
{
	Assimp::Importer importer;
	std::string filePath = "Resource/Models/" + filename + "/" + filename + ".gltf";
	const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);
	assert(scene && scene->mNumAnimations > 0);

	for (uint32_t animIndex = 0; animIndex < scene->mNumAnimations; ++animIndex) {
		aiAnimation* anim = scene->mAnimations[animIndex];
		AnimationData data;
		data.duration = float(anim->mDuration / anim->mTicksPerSecond);

		for (uint32_t channelIndex = 0; channelIndex < anim->mNumChannels; ++channelIndex) {
			aiNodeAnim* nodeAnim = anim->mChannels[channelIndex];
			NodeAnimation& node = data.nodeAnimations[nodeAnim->mNodeName.C_Str()];

			// Translate
			for (uint32_t i = 0; i < nodeAnim->mNumPositionKeys; ++i) {
				KeyframeVector3 key;
				key.time = float(nodeAnim->mPositionKeys[i].mTime / anim->mTicksPerSecond);
				key.value = { -nodeAnim->mPositionKeys[i].mValue.x, nodeAnim->mPositionKeys[i].mValue.y, nodeAnim->mPositionKeys[i].mValue.z };
				node.translate.keyframes.push_back(key);
			}

			// Scale
			for (uint32_t i = 0; i < nodeAnim->mNumScalingKeys; ++i) {
				KeyframeVector3 key;
				key.time = float(nodeAnim->mScalingKeys[i].mTime / anim->mTicksPerSecond);
				key.value = { nodeAnim->mScalingKeys[i].mValue.x, nodeAnim->mScalingKeys[i].mValue.y, nodeAnim->mScalingKeys[i].mValue.z };
				node.scale.keyframes.push_back(key);
			}

			// Rotate
			for (uint32_t i = 0; i < nodeAnim->mNumRotationKeys; ++i) {
				KeyframeQuaternion key;
				key.time = float(nodeAnim->mRotationKeys[i].mTime / anim->mTicksPerSecond);
				key.value = { -nodeAnim->mRotationKeys[i].mValue.x, nodeAnim->mRotationKeys[i].mValue.y,
							  nodeAnim->mRotationKeys[i].mValue.z, -nodeAnim->mRotationKeys[i].mValue.w };
				node.rotate.keyframes.push_back(key);
			}
		}

		// アニメーション名で登録（無名なら "Anim0", "Anim1", ...）
		std::string name = anim->mName.C_Str();
		if (name.empty()) {
			name = "Anim" + std::to_string(animIndex);
		}
		animations_[name] = std::move(data);
	}

	// 最初のアニメーションを選択
	if (!animations_.empty()) {
		currentAnimation_ = &animations_.begin()->second;
	}
}

Vector3 Animation::CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time)
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

Quaternion Animation::CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time)
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