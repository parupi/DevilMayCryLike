#include "AnimationClipSet.h"
#include <assimp/scene.h>
#include <Utility/Logger.h>
#include <World3D/Object/Model/GltfAxis.h>
#include <World3D/Object/Model/ModelLoader.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>

void AnimationClipSet::LoadFromScene(const aiScene* scene)
{
	clips_.clear();
	defaultClipName_.clear();

	// アニメーションを持たないスキンモデルもある。
	// 以前はここで assert していたが、Release では素通りして nullptr 参照になっていた
	if (!scene || scene->mNumAnimations == 0) {
		return;
	}

	for (uint32_t animIndex = 0; animIndex < scene->mNumAnimations; ++animIndex) {
		const aiAnimation* anim = scene->mAnimations[animIndex];
		const double ticksPerSecond = (anim->mTicksPerSecond != 0.0) ? anim->mTicksPerSecond : 25.0;

		AnimationData data;
		data.duration = float(anim->mDuration / ticksPerSecond);

		for (uint32_t channelIndex = 0; channelIndex < anim->mNumChannels; ++channelIndex) {
			const aiNodeAnim* nodeAnim = anim->mChannels[channelIndex];
			NodeAnimation& node = data.nodeAnimations[nodeAnim->mNodeName.C_Str()];

			// 軸変換は GltfAxis に一本化してある。ModelLoader の頂点側と必ず同じ変換を通すこと
			for (uint32_t i = 0; i < nodeAnim->mNumPositionKeys; ++i) {
				node.translate.keyframes.push_back({
					float(nodeAnim->mPositionKeys[i].mTime / ticksPerSecond),
					GltfAxis::Position(nodeAnim->mPositionKeys[i].mValue) });
			}
			for (uint32_t i = 0; i < nodeAnim->mNumScalingKeys; ++i) {
				node.scale.keyframes.push_back({
					float(nodeAnim->mScalingKeys[i].mTime / ticksPerSecond),
					GltfAxis::Scale(nodeAnim->mScalingKeys[i].mValue) });
			}
			for (uint32_t i = 0; i < nodeAnim->mNumRotationKeys; ++i) {
				node.rotate.keyframes.push_back({
					float(nodeAnim->mRotationKeys[i].mTime / ticksPerSecond),
					GltfAxis::Rotation(nodeAnim->mRotationKeys[i].mValue) });
			}
		}

		std::string name = anim->mName.C_Str();
		if (name.empty()) {
			name = "Anim" + std::to_string(animIndex);
		}
		clips_[name] = std::move(data);
	}

	if (!clips_.empty()) {
		defaultClipName_ = clips_.begin()->first;
	}
}

void AnimationClipSet::AddEvent(const std::string& clipName, float time, const std::string& tag)
{
	auto it = clips_.find(clipName);
	if (it == clips_.end()) {
		Logger::Log("[AnimationClipSet] イベントを付けるクリップがありません: " + clipName + "\n");
		return;
	}

	AnimationData& clip = it->second;
	const AnimationEvent event{ time, tag };
	// time 昇順を保つ。AnimationPlayer は区間走査で発火するので順序が崩れると取りこぼす
	auto insertAt = std::upper_bound(clip.events.begin(), clip.events.end(), event,
		[](const AnimationEvent& a, const AnimationEvent& b) { return a.time < b.time; });
	clip.events.insert(insertAt, event);
}

void AnimationClipSet::RemoveEvent(const std::string& clipName, size_t index)
{
	auto it = clips_.find(clipName);
	if (it == clips_.end()) return;
	if (index >= it->second.events.size()) return;
	it->second.events.erase(it->second.events.begin() + index);
}

void AnimationClipSet::SortEvents(const std::string& clipName)
{
	auto it = clips_.find(clipName);
	if (it == clips_.end()) return;
	std::stable_sort(it->second.events.begin(), it->second.events.end(),
		[](const AnimationEvent& a, const AnimationEvent& b) { return a.time < b.time; });
}

std::vector<AnimationEvent>* AnimationClipSet::GetEventsMutable(const std::string& clipName)
{
	auto it = clips_.find(clipName);
	return (it != clips_.end()) ? &it->second.events : nullptr;
}

bool AnimationClipSet::SaveEvents(const std::string& filename) const
{
	nlohmann::json root;
	nlohmann::json clipsJson = nlohmann::json::object();

	for (const auto& [clipName, clip] : clips_) {
		if (clip.events.empty()) continue; // イベントの無いクリップは書かない
		nlohmann::json events = nlohmann::json::array();
		for (const AnimationEvent& event : clip.events) {
			events.push_back({ {"time", event.time}, {"tag", event.tag} });
		}
		clipsJson[clipName] = events;
	}
	root["clips"] = clipsJson;

	// サブフォルダを含むモデル名（"Enemys/Dragon" など）でも正しい場所を指すよう共通の規約に合わせる。
	// ここで素朴に filename を2回繋ぐと Resource/Models/Enemys/Dragon/Enemys/Dragon.anim.json になる
	const std::string path = ModelLoader::MakeAssetBasePath(filename) + ".anim.json";
	std::ofstream file(path);
	if (!file.is_open()) {
		Logger::Log("[AnimationClipSet] 書き出しに失敗: " + path + "\n");
		return false;
	}
	file << root.dump(2);
	Logger::Log("[AnimationClipSet] イベントを保存: " + path + "\n");
	return true;
}

void AnimationClipSet::LoadEvents(const std::string& filename)
{
	// サブフォルダを含むモデル名（"Enemys/Dragon" など）でも正しい場所を指すよう共通の規約に合わせる。
	// ここで素朴に filename を2回繋ぐと Resource/Models/Enemys/Dragon/Enemys/Dragon.anim.json になる
	const std::string path = ModelLoader::MakeAssetBasePath(filename) + ".anim.json";
	std::ifstream file(path);
	if (!file.is_open()) return; // イベント定義は任意

	nlohmann::json root;
	try {
		file >> root;
	} catch (const std::exception& e) {
		Logger::Log("[AnimationClipSet] " + path + " の解析に失敗: " + e.what() + "\n");
		return;
	}

	if (!root.contains("clips") || !root["clips"].is_object()) return;

	for (const auto& [clipName, events] : root["clips"].items()) {
		if (!events.is_array()) continue;
		for (const auto& event : events) {
			if (!event.contains("time") || !event.contains("tag")) continue;
			AddEvent(clipName, event["time"].get<float>(), event["tag"].get<std::string>());
		}
	}
}

const AnimationData* AnimationClipSet::Find(const std::string& name) const
{
	auto it = clips_.find(name);
	return (it != clips_.end()) ? &it->second : nullptr;
}

const AnimationData* AnimationClipSet::GetDefaultClip() const
{
	if (clips_.empty()) return nullptr;
	return &clips_.begin()->second;
}

std::vector<std::string> AnimationClipSet::GetClipNames() const
{
	std::vector<std::string> names;
	names.reserve(clips_.size());
	for (const auto& [name, _] : clips_) {
		names.push_back(name);
	}
	return names;
}

namespace {
	// キーフレーム列から time を挟む区間を探す。
	// キーは time 昇順なので二分探索でよい（線形探索だと ジョイント数 × キー数 × 毎フレーム になる）
	template <typename T>
	size_t FindSegment(const std::vector<T>& keyframes, float time)
	{
		// time より後ろにある最初のキーを探し、その1つ前を区間の始点にする
		auto it = std::upper_bound(keyframes.begin(), keyframes.end(), time,
			[](float t, const T& key) { return t < key.time; });

		if (it == keyframes.begin()) return 0;                  // 先頭より前
		if (it == keyframes.end()) return keyframes.size();     // 最後のキーより後ろ
		return static_cast<size_t>(std::distance(keyframes.begin(), it)) - 1;
	}
}

Vector3 SampleCurve(const std::vector<KeyframeVector3>& keyframes, float time)
{
	if (keyframes.empty()) return { 0.0f, 0.0f, 0.0f };
	if (keyframes.size() == 1 || time <= keyframes[0].time) return keyframes[0].value;

	const size_t index = FindSegment(keyframes, time);
	if (index >= keyframes.size()) return keyframes.back().value;

	const float span = keyframes[index + 1].time - keyframes[index].time;
	if (span <= 0.0f) return keyframes[index].value;

	const float t = (time - keyframes[index].time) / span;
	return Lerp(keyframes[index].value, keyframes[index + 1].value, t);
}

Quaternion SampleCurve(const std::vector<KeyframeQuaternion>& keyframes, float time)
{
	if (keyframes.empty()) return Identity();
	if (keyframes.size() == 1 || time <= keyframes[0].time) return keyframes[0].value;

	const size_t index = FindSegment(keyframes, time);
	if (index >= keyframes.size()) return keyframes.back().value;

	const float span = keyframes[index + 1].time - keyframes[index].time;
	if (span <= 0.0f) return keyframes[index].value;

	const float t = (time - keyframes[index].time) / span;
	return Slerp(keyframes[index].value, keyframes[index + 1].value, t);
}
