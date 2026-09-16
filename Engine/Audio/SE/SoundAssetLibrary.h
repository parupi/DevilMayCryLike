#pragma once
#include <string>
#include <vector>

#include "AudioBuffer.h"
#include "SoundDefinition.h"

/// <summary>
/// .sound（SEエディタで作った定義）をゲームから鳴らせる形にする層（設計書 Phase 8）。
///
/// やっていることは3つだけ:
///   1. Resource/Sounds/&lt;name&gt;.sound を読む
///   2. SoundSynth で波形へ焼く（1回だけ。結果は名前で覚えておく）
///   3. Audio へ登録して、以後は普通の SoundPlayWave で鳴らせるようにする
///
/// 焼くのはゲーム実行中に一度きりなので、毎フレームの負荷にはならない。
/// シーン頭で <see cref="Preload"/> しておけば、初回再生のもたつきも無くせる。
/// </summary>
namespace SoundAssets {

	/// <summary>Resource/Sounds/ にある .sound の名前一覧（拡張子なし）</summary>
	std::vector<std::string> ListNames();

	/// <summary>その名前の .sound があるか</summary>
	bool Exists(const std::string& name);

	/// <summary>
	/// 読み込み・合成・Audio への登録をまとめて済ませる。
	/// 既に登録済みなら何もしない
	/// </summary>
	/// <returns>鳴らせる状態になったら true</returns>
	bool Preload(const std::string& name);

	/// <summary>定義に書かれた優先度。未登録なら既定値を返す</summary>
	int GetPriority(const std::string& name);

	/// <summary>
	/// エディタで編集中の定義を、その場で焼いて Audio へ入れ直す。
	/// ファイルには触らないので、保存せずに試聴できる
	/// </summary>
	/// <param name="registerName">Audio へ登録する名前。空なら definition.name</param>
	/// <returns>焼いた波形（波形表示にそのまま使える）</returns>
	AudioBuffer RenderAndRegister(const SoundDefinition& definition, const std::string& registerName = {});

	/// <summary>
	/// 焼き直しが要る状態にする。.sound を保存した直後に呼ぶと、
	/// 次の Preload で新しい内容が読み直される
	/// </summary>
	void Invalidate(const std::string& name);

	/// <summary>全部の登録を忘れる（シーンをまたいで作り直したいとき用）</summary>
	void InvalidateAll();

} // namespace SoundAssets
