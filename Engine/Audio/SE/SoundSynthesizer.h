#pragma once
#include "AudioBuffer.h"
#include "SoundDefinition.h"

/// <summary>
/// SoundDefinition から波形を焼く（設計書 §6「リアルタイム処理とオフライン生成を分ける」）。
///
/// 生成はここで全部済ませ、ゲーム実行中は出来上がったバッファを鳴らすだけにする。
/// だから ImGui スレッドから呼んでも音が途切れないし、
/// XAudio2 側のスレッドで DSP を回す必要もない。
///
/// 処理順は設計書 Phase 0 の Audio Processing Graph そのまま:
/// <code>
/// 音源 → ピッチ → 音量エンベロープ → フィルタ → （レイヤー合成） → エフェクト → 出力
/// </code>
/// </summary>
namespace SoundSynth {

	/// <summary>SE 全体を焼く。レイヤーが無い／長さ0 のときは空のバッファを返す</summary>
	AudioBuffer Render(const SoundDefinition& definition);

	/// <summary>
	/// レイヤー1本だけを焼く（モノラル）。エディタでレイヤーを単体試聴するのに使う。
	/// マスターのエフェクトは掛からない
	/// </summary>
	AudioBuffer RenderLayer(const SELayer& layer, int sampleRate);

} // namespace SoundSynth
