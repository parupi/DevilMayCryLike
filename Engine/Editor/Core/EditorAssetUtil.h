#pragma once
#ifdef _DEBUG

#include <string>
#include <vector>

class BaseModel;

/// <summary>
/// エディタからアセットを扱うための小道具。
/// ディスクの走査は毎フレームやると重いので、呼ぶ側でキャッシュすること。
/// </summary>
namespace Editor {

/// <summary>
/// Resource/Models/ を走査して、読み込めるモデル名（＝フォルダ名）を返す。
/// ModelLoader の規約に合わせて &lt;name&gt;/&lt;name&gt;.obj / .gltf / .fbx が
/// 実在するフォルダだけを拾う。存在しない名前を LoadModel に渡すと assert で落ちるため。
/// </summary>
std::vector<std::string> ScanModelFolders();

/// <summary>
/// ScanModelFolders() の結果をキャッシュして返す。毎フレーム呼んでよい。
/// モデルを足したときは rescan = true で掘り直す。
/// </summary>
const std::vector<std::string>& CachedModelFolders(bool rescan = false);

/// <summary>
/// 読み込み済みモデルの実体から、その登録名を逆引きする。
/// BaseRenderer が持っているのはポインタだけなので、表示や複製にはこれが要る。
/// 見つからなければ空文字。
/// </summary>
std::string FindLoadedModelName(const BaseModel* model);

} // namespace Editor

#endif // _DEBUG
