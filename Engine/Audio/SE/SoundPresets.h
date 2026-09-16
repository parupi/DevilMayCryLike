#pragma once
#include <string>
#include <vector>

#include "SoundDefinition.h"

/// <summary>
/// 組み込みプリセット（設計書 Phase 6「プリセット」と §5「最初の制作目標」）。
///
/// 白紙から音を作るのは経験が要るので、代表的な SE をコードで持っておき、
/// エディタからはそれを土台に触ってもらう。
/// 「書き出し」を押すと Resource/Sounds/&lt;名前&gt;.sound になり、以後は普通の編集対象になる。
/// </summary>
namespace SoundPresets {

	int Count();
	/// <summary>プリセット名（そのまま .sound のファイル名になる）</summary>
	const char* GetName(int index);
	/// <summary>そのプリセットが何の音か。エディタの説明に出す</summary>
	const char* GetDescription(int index);

	/// <summary>index 番のプリセットを作る。範囲外なら空の定義</summary>
	SoundDefinition Create(int index);
	/// <summary>名前で作る。見つからなければ false</summary>
	bool CreateByName(const std::string& name, SoundDefinition& outDefinition);

	/// <summary>レイヤー1本だけを持つ最小の定義。「新規作成」の初期値</summary>
	SoundDefinition CreateEmpty(const std::string& name);

	/// <summary>
	/// 全プリセットを Resource/Sounds へ書き出す。
	///
	/// 次のものは飛ばす:
	/// 　・既にある .sound（overwrite が true のときを除く。編集済みを潰さないため）
	/// 　・同じ名前の .wav 素材があるもの（SwordSlash など。ゲームの音が黙って差し替わるのを防ぐ）
	/// </summary>
	/// <returns>実際に書き出した数</returns>
	int ExportAll(bool overwrite);

} // namespace SoundPresets
