#pragma once
#include <Math/Vector3.h>
#include <string>
#include <vector>

/// <summary>
/// 遊べる敵の一覧。
///
/// これまで敵の種類は GameObjectRegister.cpp の Object3dFactory::Register が並んでいるだけで、
/// 「プレイヤーが選べる敵はどれか」「画面に何と出すか」を引ける場所が無かった。
/// トレーニングの選択メニューはここを回して作る。敵を増やしたらここへ1行足す
/// （ファクトリへの登録は従来どおり GameObjectRegister.cpp にも必要）。
/// </summary>
struct EnemyCatalogEntry {
	/// <summary>Object3dFactory の登録キー。ステージデータの "class" と同じもの</summary>
	std::string className;
	/// <summary>メニューに出す名前</summary>
	std::string displayName;
	/// <summary>1行の説明。選択中の項目の下に出す</summary>
	std::string description;

	/// <summary>
	/// 実行中に生成するときに付ける本体コライダーの大きさ。
	/// ステージデータに書かれている値と同じにしてある（BossKnight のように
	/// Initialize() の中でさらに倍率を掛けるクラスがあるので、素の値を持つこと）。
	///
	/// BossKnight::Initialize() は GetCollider(name_) を null チェック無しで参照するため、
	/// 実行中に敵を作るときは必ず Initialize() の前にコライダーを付けること。
	/// </summary>
	Vector3 colliderHalfExtents{ 1.0f, 1.0f, 1.0f };

	/// <summary>プレイヤーからどれだけ離して出すか（m）。大きい敵ほど遠くに置く</summary>
	float spawnDistance = 8.0f;
};

namespace EnemyCatalog {

/// <summary>並び順がそのままメニューの並びになる</summary>
const std::vector<EnemyCatalogEntry>& GetEntries();

/// <summary>クラス名から引く。無ければ nullptr</summary>
const EnemyCatalogEntry* Find(const std::string& className);

/// <summary>既定で選ばれる敵のクラス名（一覧の先頭）</summary>
const std::string& GetDefaultClassName();

} // namespace EnemyCatalog
