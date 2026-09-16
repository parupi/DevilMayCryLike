#include "SoundSynthesizer.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

	// フィルタのカットオフを掃引するとき、係数を何サンプルごとに作り直すか。
	// 毎サンプル作り直しても正しいが三角関数が4回入る。32 サンプル（44.1kHz で 0.7ms）なら
	// 耳では段差が分からず、生成時間が目に見えて短くなる
	constexpr int kFilterUpdateInterval = 32;

	/// <summary>レイヤー1本をモノラルで焼く。長さは layer.duration ぶん</summary>
	std::vector<float> RenderLayerMono(const SELayer& layer, int sampleRate)
	{
		const float duration = (std::max)(layer.duration, 0.0f);
		if (duration <= 0.0f || sampleRate <= 0) { return {}; }

		const size_t frameCount = static_cast<size_t>(duration * static_cast<float>(sampleRate));
		if (frameCount == 0) { return {}; }

		std::vector<float> output(frameCount, 0.0f);

		SoundDSP::Oscillator oscillator;
		oscillator.SetWaveType(layer.wave);
		oscillator.SetSampleRate(static_cast<float>(sampleRate));
		oscillator.SetPulseWidth(layer.pulseWidth);
		oscillator.SetFM(layer.fmRatio, layer.fmAmount);
		oscillator.Reset(static_cast<uint32_t>(layer.noiseSeed));

		SoundDSP::Biquad filter;
		filter.Configure(layer.filterType, static_cast<float>(sampleRate), layer.filterCutoff, layer.filterResonance);

		// 0 は「レイヤー全体を使う」の意味。設計書の Pitch Envelope はほぼこの形
		const float sweepTime = (layer.pitchSweepTime > 0.0f) ? layer.pitchSweepTime : duration;
		const bool sweepFilter = layer.filterSweepEnabled && layer.filterType != SoundDSP::FilterType::None;

		for (size_t i = 0; i < frameCount; ++i) {
			const float t = static_cast<float>(i) / static_cast<float>(sampleRate);

			// ── ピッチ ──
			float frequency = layer.frequency;
			if (layer.pitchSweepEnabled) {
				const float progress = (sweepTime > 0.0f) ? (t / sweepTime) : 1.0f;
				frequency = SoundDSP::SweepValue(layer.pitchStart, layer.pitchEnd, progress, layer.pitchCurve);
			}
			oscillator.SetFrequency(frequency);

			// ── 音源 ──
			float sample = oscillator.Next();

			// ── 音量エンベロープ ──
			sample *= layer.envelope.Evaluate(t, duration);

			// ── フィルタ ──
			if (sweepFilter && (i % kFilterUpdateInterval) == 0) {
				const float progress = (duration > 0.0f) ? (t / duration) : 1.0f;
				const float cutoff = SoundDSP::SweepValue(
					layer.filterCutoff, layer.filterCutoffEnd, progress, SoundDSP::SweepCurve::Exponential);
				filter.Configure(layer.filterType, static_cast<float>(sampleRate), cutoff, layer.filterResonance);
			}
			output[i] = filter.Process(sample) * layer.volume;
		}

		return output;
	}

} // namespace

namespace SoundSynth {

	AudioBuffer RenderLayer(const SELayer& layer, int sampleRate)
	{
		AudioBuffer buffer;
		buffer.sampleRate = sampleRate;
		buffer.channels = 1;
		buffer.samples = RenderLayerMono(layer, sampleRate);
		return buffer;
	}

	AudioBuffer Render(const SoundDefinition& definition)
	{
		AudioBuffer buffer;
		// XAudio2 のマスターボイスに合わせて 8k〜192kHz の常識的な範囲に収める
		buffer.sampleRate = std::clamp(definition.sampleRate, 8000, 192000);
		buffer.channels = 2;

		const float totalDuration = definition.GetTotalDuration();
		if (totalDuration <= 0.0f || definition.layers.empty()) { return buffer; }

		const size_t frameCount = static_cast<size_t>(totalDuration * static_cast<float>(buffer.sampleRate));
		if (frameCount == 0) { return buffer; }
		buffer.Resize(frameCount);

		// ── レイヤーを焼いて、開始時間ぶんずらしながら混ぜる（Phase 5）──
		for (const SELayer& layer : definition.layers) {
			if (!layer.enabled) { continue; }

			const std::vector<float> mono = RenderLayerMono(layer, buffer.sampleRate);
			if (mono.empty()) { continue; }

			const size_t offset = static_cast<size_t>(
				(std::max)(layer.startDelay, 0.0f) * static_cast<float>(buffer.sampleRate));

			// 定位。中央で左右とも 1 倍にすると、パンを振ったときだけ音が痩せて聞こえる。
			// SE では等出力（-3dB センター）よりも素直なこちらのほうが扱いやすい
			const float pan = std::clamp(layer.pan, -1.0f, 1.0f);
			const float leftGain = (pan <= 0.0f) ? 1.0f : (1.0f - pan);
			const float rightGain = (pan >= 0.0f) ? 1.0f : (1.0f + pan);

			for (size_t i = 0; i < mono.size(); ++i) {
				const size_t frame = offset + i;
				if (frame >= frameCount) { break; }
				buffer.samples[frame * 2 + 0] += mono[i] * leftGain;
				buffer.samples[frame * 2 + 1] += mono[i] * rightGain;
			}
		}

		// ── マスター：歪み → ディレイ → リバーブ → フィルタ（Phase 4）──
		if (definition.distortion.enabled) {
			for (float& sample : buffer.samples) {
				sample = SoundDSP::Distort(sample, definition.distortion.drive, definition.distortion.mix);
			}
		}
		if (definition.delay.enabled) {
			SoundDSP::ApplyDelay(buffer.samples, buffer.channels, buffer.sampleRate,
				definition.delay.time, definition.delay.feedback, definition.delay.mix);
		}
		if (definition.reverb.enabled) {
			SoundDSP::ApplyReverb(buffer.samples, buffer.channels, buffer.sampleRate,
				definition.reverb.roomSize, definition.reverb.damping,
				definition.reverb.width, definition.reverb.mix);
		}
		if (definition.masterFilterType != SoundDSP::FilterType::None) {
			// 左右で状態を分けないと片側の履歴がもう片側へ漏れる
			SoundDSP::Biquad filters[2];
			for (SoundDSP::Biquad& filter : filters) {
				filter.Configure(definition.masterFilterType, static_cast<float>(buffer.sampleRate),
					definition.masterFilterCutoff, definition.masterFilterResonance);
			}
			for (size_t frame = 0; frame < frameCount; ++frame) {
				for (int ch = 0; ch < 2; ++ch) {
					float& sample = buffer.samples[frame * 2 + ch];
					sample = filters[ch].Process(sample);
				}
			}
		}

		// ── 仕上げ ──
		// 直流は正規化より先に抜く。偏ったまま揃えると、そのぶん本体が小さくなる
		SoundDSP::RemoveDCOffset(buffer.samples, buffer.channels, buffer.sampleRate);
		// 正規化はマスター音量より先に掛ける。順番が逆だと音量つまみが無効になる
		if (definition.normalize) {
			buffer.NormalizeTo(std::clamp(definition.normalizePeak, 0.01f, 1.0f));
		}
		if (definition.masterVolume != 1.0f) {
			for (float& sample : buffer.samples) {
				sample *= definition.masterVolume;
			}
		}
		buffer.ClipToRange();
		// 先頭・末尾が 0 でないまま切れるとプチッと鳴る
		buffer.ApplyEdgeFade();

		return buffer;
	}

} // namespace SoundSynth
