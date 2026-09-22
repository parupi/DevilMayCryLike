#pragma once
#include <string>
#include <vector>
#include <Math/Quaternion.h>
#include <Math/Vector3.h>
#include "Skeleton.h"

/// <summary>
/// 補正1つぶん。「どのボーンを・どれだけ・どの空間で回すか」だけを持つ
/// </summary>
struct BoneRotationEntry {
	std::string jointName;
	Quaternion rotation = Identity();
	// 0 で補正なし、1 で指定した回転そのまま。間は単位クォータニオンからの Slerp
	float weight = 1.0f;
	BoneRotationSpace space = BoneRotationSpace::Local;
};

/// <summary>
/// アニメーションが作ったポーズの上から、名前で指定したボーンへ回転を足す。
///
/// 使いどころは「1本しかない斬りクリップを、技ごとに違う振りに見せる」こと。
/// クリップを作り足さずに、腕を上げる・体をひねる・剣先を敵へ向けるといった差を付けられる。
///
/// SkinnedInstance が1つ持っていて、AnimationPlayer がポーズを作った後・
/// 行列パレットへ書き込む前に Apply される。ゲーム側は毎フレーム
/// Clear() してから必要なぶんだけ Set すればよい（登録は使い捨てで構わない）。
///
/// ウェイトは「補正なし → 補正あり」の Slerp で掛ける。
/// アニメーションの回転との Slerp にすると、触っていない軸まで補正側へ引っぱられる。
/// </summary>
class BoneModifier
{
public:
	/// <summary>
	/// 補正を登録する。同じボーン名がすでにあれば上書きする
	/// </summary>
	void Set(const std::string& jointName, const Quaternion& rotation,
		float weight = 1.0f, BoneRotationSpace space = BoneRotationSpace::Local);

	/// <summary>
	/// オイラー角[度]で登録する。エディタや攻撃データからはこちらを使う
	/// </summary>
	void SetEuler(const std::string& jointName, const Vector3& degrees,
		float weight = 1.0f, BoneRotationSpace space = BoneRotationSpace::Local);

	void Remove(const std::string& jointName);
	void Clear() { entries_.clear(); }
	bool Empty() const { return entries_.empty(); }

	// まとめて切る。登録内容は残るので、戻せば同じ補正が復活する
	void SetEnabled(bool enabled) { enabled_ = enabled; }
	bool IsEnabled() const { return enabled_; }

	/// <summary>
	/// 全体に掛かる倍率。攻撃の出入りで補正を滑らかに効かせるのに使う
	/// </summary>
	void SetGlobalWeight(float weight) { globalWeight_ = weight; }
	float GetGlobalWeight() const { return globalWeight_; }

	/// <summary>
	/// スケルトンへ適用する。ローカル回転しか触らないので、
	/// 呼んだ側が最後に Skeleton::Update() を回すこと。
	/// 1つでも適用したら true を返す
	/// </summary>
	bool Apply(Skeleton& skeleton) const;

	// エディタ用。ゲーム側は Set / Remove を使うこと
	std::vector<BoneRotationEntry>& GetEntries() { return entries_; }
	const std::vector<BoneRotationEntry>& GetEntries() const { return entries_; }

private:
	std::vector<BoneRotationEntry> entries_;
	bool enabled_ = true;
	float globalWeight_ = 1.0f;
};
