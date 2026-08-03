#pragma once
#include <string>
#include <vector>

/// <summary>
/// 「いまどのステージを編集・再生しているか」を保持する。
/// GameScene が読み込む先も、エディタが保存する先もここを見る。
///
/// 既定は SceneLoader::kDefaultStagePath。エディタの Stage メニューから
/// 「名前を付けて保存」「開く」で切り替える。
/// 切り替えた先が設定ファイルに残るのは Debug ビルドだけで、
/// Release では常に既定のステージを読む（テスト用のステージが製品に混ざらないように）。
/// </summary>
namespace StageDocument {

// ステージデータの置き場と拡張子
constexpr const char* kDirectory = "Resource/Stage";
constexpr const char* kExtension = ".json";

/// <summary>現在のステージのパス</summary>
const std::string& GetPath();
void SetPath(const std::string& path);

/// <summary>現在のステージのファイル名（拡張子なし）</summary>
std::string GetName();

/// <summary>"Stage2" → "Resource/Stage/Stage2.json"</summary>
std::string MakePath(const std::string& name);

/// <summary>Resource/Stage にある .json のファイル名（拡張子なし）を辞書順で返す</summary>
std::vector<std::string> ListNames();

/// <summary>
/// ファイル名として使えるか。空・パス区切り・ドライブ指定を弾く。
/// エディタの入力欄からそのままファイルを作るので、ここで止めておく
/// </summary>
bool IsValidName(const std::string& name);

/// <summary>その名前のステージが既にあるか</summary>
bool Exists(const std::string& name);

} // namespace StageDocument
