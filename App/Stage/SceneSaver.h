#pragma once
#include <string>
#include <vector>
#include <Stage/SceneLoader.h>

class Object3d;

/// <summary>
/// 現在シーンにあるオブジェクトを、そのままステージデータ（エンジン空間）へ書き出す。
///
/// 保存対象は `Object3d::IsStageObject()` が true のものだけ。
/// 敵の武器のように実行中に生えるオブジェクトを巻き込まないための線引きで、
/// フラグを立てるのは SceneBuilder とエディタの生成経路だけ。
/// </summary>
class SceneSaver
{
public:
	/// <summary>現在シーンを path へ保存する。失敗したら false（理由は Logger へ）</summary>
	static bool Save(const std::string& path);

	/// <summary>現在シーンをステージデータに変換する（保存せずに中身を見たいとき用）</summary>
	static std::vector<SceneObject> Capture();

private:
	static SceneObject CaptureObject(Object3d* object);
};
