#pragma once
#include <assimp/types.h>
#include <Math/Vector3.h>
#include <Math/Quaternion.h>

/// <summary>
/// gltf(右手系 Y-up) からこのエンジン(左手系) へ持ってくるときの軸変換。
///
/// 頂点・ノード階層・バインドポーズ・アニメーションのキーフレームは
/// **すべて同じ変換を通っていないといけない**。以前は ModelLoader と
/// アニメーション読み込みに別々の式がハードコードされていて、
/// 片方だけ直すとモデルとアニメがずれる状態だった。
///
/// クォータニオンは q と -q が同じ回転を表すので、
/// 旧アニメーション側の { -x, y, z, -w } は Rotation() の符号違い（同じ回転）。
/// Slerp が dot<0 で反転して最短経路を取るため、符号を揃えても補間結果は変わらない。
/// </summary>
namespace GltfAxis {

	// 位置・法線は X を反転する
	inline Vector3 Position(const aiVector3D& v) { return { -v.x, v.y, v.z }; }
	inline Vector3 Normal(const aiVector3D& v) { return { -v.x, v.y, v.z }; }
	// スケールは軸の入れ替えが無いのでそのまま
	inline Vector3 Scale(const aiVector3D& v) { return { v.x, v.y, v.z }; }
	// X を反転した結果、回転方向が逆になるので Y/Z 軸も反転する
	inline Quaternion Rotation(const aiQuaternion& q) { return { q.x, -q.y, -q.z, q.w }; }

} // namespace GltfAxis
