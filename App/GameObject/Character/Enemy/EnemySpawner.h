#pragma once
#include <Math/Vector3.h>
#include <string>

class Enemy;

/// <summary>
/// ステージデータを介さず、実行中に敵を1体作る。
///
/// ステージから作る経路（SceneBuilder）はコライダーを JSON から付けてくれるが、
/// 実行中に作るときは自分で用意しないといけない。
/// BossKnight::Initialize() は本体コライダーを null チェック無しで参照するので、
/// 「コライダーを付けてから Initialize()」の順を守るのがこの関数の主な仕事。
/// </summary>
namespace EnemySpawner {

/// <summary>
/// 敵を作って Object3dManager に登録する。
///
/// 出現演出は始めないので、呼んだ側で Enemy::Spawn() を呼ぶこと
/// （ロックオン登録など、演出前に済ませたい配線があるため分けてある）。
/// </summary>
/// <param name="className">Object3dFactory の登録キー（EnemyCatalog の className）</param>
/// <param name="objectName">オブジェクト名。FindObject で引くときの名前になる</param>
/// <param name="colliderHalfExtents">本体に付ける OBB コライダーの大きさ</param>
/// <returns>作った敵。className が敵クラスでなければ nullptr（この場合は何も追加しない）</returns>
Enemy* Spawn(const std::string& className, const std::string& objectName,
	const Vector3& position, const Vector3& colliderHalfExtents);

} // namespace EnemySpawner
