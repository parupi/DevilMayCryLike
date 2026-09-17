#pragma once
#include <cstdint>
#include <string>
#include <vector>

/// <summary>
/// 合成結果を持ち回すための、装置に依存しない波形バッファ（設計書 Phase 1）。
///
/// XAudio2 が受け取るのは 16bit PCM のバイト列だが、途中の加工を全部そこでやると
/// クリップと丸めが積み重なる。合成中は float（-1〜1）で通し、
/// 鳴らす直前に <see cref="ToPCM16"/> で一度だけ量子化する。
///
/// サンプルは**インターリーブ**で入っている（ステレオなら L,R,L,R...）。
/// </summary>
struct AudioBuffer
{
	std::vector<float> samples;
	int sampleRate = 44100;
	int channels = 2;

	size_t FrameCount() const { return channels > 0 ? samples.size() / channels : 0; }
	float DurationSeconds() const;

	/// <summary>フレーム数を指定して0で確保し直す</summary>
	void Resize(size_t frameCount);
	void Clear();
	bool IsEmpty() const { return samples.empty(); }

	/// <summary>フレーム f のチャンネル ch。範囲外は 0 を返す（読み出し専用の安全版）</summary>
	float Sample(size_t frame, int channel) const;

	/// <summary>絶対値の最大。0 なら無音</summary>
	float PeakLevel() const;
	/// <summary>RMS（体感音量の目安）</summary>
	float RmsLevel() const;

	/// <summary>ピークが target になるよう全体を掛ける。無音なら何もしない</summary>
	void NormalizeTo(float targetPeak);
	/// <summary>-1〜1 に収める</summary>
	void ClipToRange();
	/// <summary>先頭と末尾に短いフェードを掛けてプチッというノイズを消す</summary>
	void ApplyEdgeFade(float seconds = 0.002f);

	/// <summary>XAudio2 に渡す 16bit PCM のバイト列</summary>
	std::vector<uint8_t> ToPCM16() const;

	/// <summary>16bit PCM の .wav として書き出す。親ディレクトリが無ければ作る</summary>
	bool WriteWavFile(const std::string& filePath) const;
};
