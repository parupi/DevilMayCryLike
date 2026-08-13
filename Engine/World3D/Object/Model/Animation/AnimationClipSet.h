#pragma once
#include <map>
#include <string>
#include <vector>
#include <Math/Vector3.h>
#include <Math/Quaternion.h>
#include "World3D/Object/Model/ModelStructs.h"

struct aiScene;

// 任意の時刻のキーフレーム値を取り出す
Vector3 SampleCurve(const std::vector<KeyframeVector3>& keyframes, float time);
Quaternion SampleCurve(const std::vector<KeyframeQuaternion>& keyframes, float time);

/// <summary>
/// モデル1つぶんのアニメーションクリップ集合。
///
/// これは「アセット」であって再生状態を一切持たない。ModelManager が抱える
/// SkinnedModel が所有し、全インスタンスで共有される。
/// 再生位置やブレンドの状態は AnimationPlayer 側（インスタンスごと）が持つ。
/// </summary>
class AnimationClipSet
{
public:
	/// <summary>
	/// 読み込み済みの aiScene からクリップを取り出す。アニメーションが無いモデルでも失敗にはしない。
	/// gltf を2回開かないよう、モデル本体と同じ scene を ModelLoader から渡してもらう
	/// </summary>
	void LoadFromScene(const aiScene* scene);

	/// <summary>
	/// Resource/Models/&lt;filename&gt;/&lt;filename&gt;.anim.json があればイベントを読む。
	/// クリップを読み終えた後に呼ぶこと（イベントは既存クリップに足す形なので）
	/// </summary>
	void LoadEvents(const std::string& filename);

	/// <summary>
	/// クリップにイベントを1つ足す。json を用意するまでもないものはコードから直接呼べる。
	/// 同じ時刻の重複は許す（別タグを同時に鳴らしたいことがある）
	/// </summary>
	void AddEvent(const std::string& clipName, float time, const std::string& tag);

	// index 番目のイベントを消す。範囲外は無視
	void RemoveEvent(const std::string& clipName, size_t index);
	// 時刻を編集した後に呼ぶ。time 昇順に並べ直す
	void SortEvents(const std::string& clipName);
	// 編集用。エディタ以外からは触らないこと
	std::vector<AnimationEvent>* GetEventsMutable(const std::string& clipName);

	/// <summary>
	/// 現在のイベント定義を Resource/Models/&lt;filename&gt;/&lt;filename&gt;.anim.json へ書き出す。
	/// 次回起動時は LoadEvents がこれを読む
	/// </summary>
	bool SaveEvents(const std::string& filename) const;

	// 見つからなければ nullptr
	const AnimationData* Find(const std::string& name) const;

	// 最初のクリップ（名前順）。1つも無ければ nullptr
	const AnimationData* GetDefaultClip() const;
	const std::string& GetDefaultClipName() const { return defaultClipName_; }

	bool Empty() const { return clips_.empty(); }
	size_t Size() const { return clips_.size(); }
	// エディタ・デバッグ用の一覧
	std::vector<std::string> GetClipNames() const;

private:
	std::map<std::string, AnimationData> clips_;
	std::string defaultClipName_;
};
