#include "SoundDSP.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstring>

namespace {

	constexpr float kPi = 3.14159265358979323846f;
	constexpr float kTwoPi = kPi * 2.0f;

	// xorshift32。std::mt19937 を毎ボイス持つほどの品質は要らないうえ、
	// 同じシードなら必ず同じ音になってほしいので自前で持つ
	inline uint32_t NextRandom(uint32_t& state)
	{
		state ^= state << 13;
		state ^= state >> 17;
		state ^= state << 5;
		return state;
	}

	inline float RandomBipolar(uint32_t& state)
	{
		return static_cast<float>(NextRandom(state)) / 2147483648.0f - 1.0f;
	}

	const char* const kWaveNames[] = {
		"Sine", "Square", "Triangle", "Saw", "WhiteNoise", "PinkNoise",
	};
	const char* const kSweepNames[] = {
		"Linear", "Exponential", "EaseOut", "EaseIn",
	};
	const char* const kFilterNames[] = {
		"None", "LowPass", "HighPass", "BandPass",
	};

} // namespace

namespace SoundDSP {

	// ───────────────────────────── 名前 ─────────────────────────────

	const char* ToString(WaveType type)
	{
		const int index = static_cast<int>(type);
		if (index < 0 || index >= WaveTypeCount()) { return kWaveNames[0]; }
		return kWaveNames[index];
	}

	WaveType WaveFromString(const char* name)
	{
		if (!name) { return WaveType::Sine; }
		for (int i = 0; i < WaveTypeCount(); ++i) {
			if (std::strcmp(kWaveNames[i], name) == 0) { return static_cast<WaveType>(i); }
		}
		return WaveType::Sine;
	}

	const char* const* WaveTypeNames() { return kWaveNames; }
	int WaveTypeCount() { return static_cast<int>(std::size(kWaveNames)); }

	const char* ToString(SweepCurve curve)
	{
		const int index = static_cast<int>(curve);
		if (index < 0 || index >= SweepCurveCount()) { return kSweepNames[0]; }
		return kSweepNames[index];
	}

	SweepCurve SweepFromString(const char* name)
	{
		if (!name) { return SweepCurve::Exponential; }
		for (int i = 0; i < SweepCurveCount(); ++i) {
			if (std::strcmp(kSweepNames[i], name) == 0) { return static_cast<SweepCurve>(i); }
		}
		return SweepCurve::Exponential;
	}

	const char* const* SweepCurveNames() { return kSweepNames; }
	int SweepCurveCount() { return static_cast<int>(std::size(kSweepNames)); }

	const char* ToString(FilterType type)
	{
		const int index = static_cast<int>(type);
		if (index < 0 || index >= FilterTypeCount()) { return kFilterNames[0]; }
		return kFilterNames[index];
	}

	FilterType FilterFromString(const char* name)
	{
		if (!name) { return FilterType::None; }
		for (int i = 0; i < FilterTypeCount(); ++i) {
			if (std::strcmp(kFilterNames[i], name) == 0) { return static_cast<FilterType>(i); }
		}
		return FilterType::None;
	}

	const char* const* FilterTypeNames() { return kFilterNames; }
	int FilterTypeCount() { return static_cast<int>(std::size(kFilterNames)); }

	// ───────────────────────────── Oscillator ─────────────────────────────

	void Oscillator::Reset(uint32_t seed)
	{
		phase_ = 0.0f;
		fmPhase_ = 0.0f;
		previousNoise_ = 0.0f;
		heldNoise_ = 0.0f;
		// シードが 0 だと xorshift が 0 から動かなくなる
		rngState_ = (seed == 0u) ? 1u : seed;
		std::memset(pink_, 0, sizeof(pink_));
		noiseStarted_ = false;
	}

	float Oscillator::NextNoiseSample()
	{
		const float white = RandomBipolar(rngState_);
		if (waveType_ == WaveType::WhiteNoise) { return white; }

		// ピンクノイズ（Paul Kellet の近似）。白より低域が持ち上がるので
		// 爆発・地響きのような重いノイズに向く
		pink_[0] = 0.99886f * pink_[0] + white * 0.0555179f;
		pink_[1] = 0.99332f * pink_[1] + white * 0.0750759f;
		pink_[2] = 0.96900f * pink_[2] + white * 0.1538520f;
		pink_[3] = 0.86650f * pink_[3] + white * 0.3104856f;
		pink_[4] = 0.55000f * pink_[4] + white * 0.5329522f;
		pink_[5] = -0.7616f * pink_[5] - white * 0.0168980f;
		const float pink = pink_[0] + pink_[1] + pink_[2] + pink_[3] + pink_[4] + pink_[5] + pink_[6] + white * 0.5362f;
		pink_[6] = white * 0.115926f;
		return std::clamp(pink * 0.5f, -1.0f, 1.0f);
	}

	float Oscillator::Next()
	{
		if (sampleRate_ <= 0.0f) { return 0.0f; }

		// ナイキスト以上は折り返して汚くなるだけなので頭打ちにする
		const float frequency = std::clamp(frequency_, 0.0f, sampleRate_ * 0.5f - 1.0f);

		const bool isNoise = (waveType_ == WaveType::WhiteNoise || waveType_ == WaveType::PinkNoise);
		if (isNoise) {
			// 指定の周波数で乱数を引き直し、その間を線形で補間する（バリューノイズ）。
			//
			// 毎サンプル乱数にするとピッチスイープが一切効かず、設計書の Dodge
			// （Noise → Pitch Down）が作れない。かといって値を保持するだけ（サンプル&ホールド）だと
			// 値が飛ぶ瞬間の段差が広帯域のパチッという音になり、周波数を下げても高域が残る。
			// 実測でも S&H は 8000Hz→250Hz と 32 倍動かして重心が 6480→3366Hz しか下がらなかった。
			// 補間して段差を無くすと、周波数がそのまま音の明るさになる
			if (!noiseStarted_) {
				previousNoise_ = NextNoiseSample();
				heldNoise_ = NextNoiseSample();
				noiseStarted_ = true;
			}

			const float value = previousNoise_ + (heldNoise_ - previousNoise_) * phase_;

			phase_ += frequency / sampleRate_;
			while (phase_ >= 1.0f) {
				phase_ -= 1.0f;
				previousNoise_ = heldNoise_;
				heldNoise_ = NextNoiseSample();
			}
			return value;
		}

		// FM。搬送波の位相へ直接足し込む（位相変調）
		float phaseOffset = 0.0f;
		if (fmAmount_ > 0.0f && fmRatio_ > 0.0f) {
			fmPhase_ += frequency * fmRatio_ / sampleRate_;
			fmPhase_ -= std::floor(fmPhase_);
			phaseOffset = std::sin(fmPhase_ * kTwoPi) * fmAmount_;
		}

		float p = phase_ + phaseOffset;
		p -= std::floor(p);

		float value = 0.0f;
		switch (waveType_) {
		case WaveType::Square:
			value = (p < std::clamp(pulseWidth_, 0.01f, 0.99f)) ? 1.0f : -1.0f;
			break;
		case WaveType::Triangle:
			value = (p < 0.5f) ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p);
			break;
		case WaveType::Saw:
			value = 2.0f * p - 1.0f;
			break;
		case WaveType::Sine:
		default:
			value = std::sin(p * kTwoPi);
			break;
		}

		phase_ += frequency / sampleRate_;
		phase_ -= std::floor(phase_);
		return value;
	}

	void Oscillator::Process(float* output, int sampleCount)
	{
		if (!output) { return; }
		for (int i = 0; i < sampleCount; ++i) {
			output[i] = Next();
		}
	}

	// ───────────────────────────── Envelope ─────────────────────────────

	float Envelope::Evaluate(float t, float duration) const
	{
		if (duration <= 0.0f || t < 0.0f || t > duration) { return 0.0f; }

		float a = (std::max)(attack, 0.0f);
		float h = (std::max)(hold, 0.0f);
		float d = (std::max)(decay, 0.0f);
		float r = (std::max)(release, 0.0f);
		const float level = std::clamp(sustain, 0.0f, 1.0f);
		const float shape = (std::max)(curve, 0.05f);

		// A+H+D+R が音の長さを超えたら、比率を保ったまま全体を縮める。
		// こうしておくと Duration スライダーひとつで長さを詰められる
		const float fixedTotal = a + h + d + r;
		if (fixedTotal > duration && fixedTotal > 0.0f) {
			const float scale = duration / fixedTotal;
			a *= scale; h *= scale; d *= scale; r *= scale;
		}

		const float attackEnd = a;
		const float holdEnd = attackEnd + h;
		const float decayEnd = holdEnd + d;
		const float releaseStart = duration - r;

		// リリースは他の区間より優先する（短い音では区間同士が重なるため）
		if (r > 0.0f && t >= releaseStart) {
			// リリース開始時点の値を求めてから 0 へ落とす
			float startLevel = level;
			if (releaseStart < attackEnd) {
				startLevel = (a > 0.0f) ? std::clamp(releaseStart / a, 0.0f, 1.0f) : 1.0f;
			} else if (releaseStart < holdEnd) {
				startLevel = 1.0f;
			} else if (releaseStart < decayEnd) {
				const float decayProgress = (d > 0.0f) ? (releaseStart - holdEnd) / d : 1.0f;
				startLevel = 1.0f + (level - 1.0f) * std::pow(std::clamp(decayProgress, 0.0f, 1.0f), shape);
			}
			const float progress = std::clamp((t - releaseStart) / r, 0.0f, 1.0f);
			return startLevel * (1.0f - std::pow(progress, 1.0f / shape));
		}

		if (t < attackEnd) {
			return (a > 0.0f) ? (t / a) : 1.0f;
		}
		if (t < holdEnd) {
			return 1.0f;
		}
		if (t < decayEnd) {
			const float progress = (d > 0.0f) ? (t - holdEnd) / d : 1.0f;
			return 1.0f + (level - 1.0f) * std::pow(std::clamp(progress, 0.0f, 1.0f), shape);
		}
		return level;
	}

	// ───────────────────────────── Sweep ─────────────────────────────

	float ApplySweepCurve(SweepCurve curve, float t01)
	{
		const float t = std::clamp(t01, 0.0f, 1.0f);
		switch (curve) {
		case SweepCurve::EaseOut: return 1.0f - (1.0f - t) * (1.0f - t);
		case SweepCurve::EaseIn:  return t * t;
		case SweepCurve::Linear:
		case SweepCurve::Exponential:
		default:                  return t;
		}
	}

	float SweepValue(float from, float to, float t01, SweepCurve curve)
	{
		const float t = std::clamp(t01, 0.0f, 1.0f);

		if (curve == SweepCurve::Exponential && from > 1.0e-4f && to > 1.0e-4f) {
			// 周波数は比で聞こえるので、対数上を真っ直ぐ動かすほうが自然に下がって聞こえる
			return from * std::pow(to / from, t);
		}
		return from + (to - from) * ApplySweepCurve(curve, t);
	}

	// ───────────────────────────── Biquad ─────────────────────────────

	void Biquad::Configure(FilterType type, float sampleRate, float cutoff, float q)
	{
		passthrough_ = (type == FilterType::None);
		if (passthrough_ || sampleRate <= 0.0f) { return; }

		// カットオフをナイキストの手前で止める。ここを超えると係数が発散する
		const float nyquist = sampleRate * 0.5f;
		const float clampedCutoff = std::clamp(cutoff, 10.0f, nyquist * 0.99f);
		const float clampedQ = std::clamp(q, 0.05f, 20.0f);

		const float omega = kTwoPi * clampedCutoff / sampleRate;
		const float sinOmega = std::sin(omega);
		const float cosOmega = std::cos(omega);
		const float alpha = sinOmega / (2.0f * clampedQ);

		float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
		const float a0 = 1.0f + alpha;
		const float a1 = -2.0f * cosOmega;
		const float a2 = 1.0f - alpha;

		switch (type) {
		case FilterType::LowPass:
			b0 = (1.0f - cosOmega) * 0.5f;
			b1 = 1.0f - cosOmega;
			b2 = b0;
			break;
		case FilterType::HighPass:
			b0 = (1.0f + cosOmega) * 0.5f;
			b1 = -(1.0f + cosOmega);
			b2 = b0;
			break;
		case FilterType::BandPass:
			b0 = alpha;
			b1 = 0.0f;
			b2 = -alpha;
			break;
		default:
			passthrough_ = true;
			return;
		}

		b0_ = b0 / a0;
		b1_ = b1 / a0;
		b2_ = b2 / a0;
		a1_ = a1 / a0;
		a2_ = a2 / a0;
	}

	void Biquad::Reset()
	{
		x1_ = x2_ = y1_ = y2_ = 0.0f;
	}

	float Biquad::Process(float input)
	{
		if (passthrough_) { return input; }

		const float output = b0_ * input + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;
		x2_ = x1_;
		x1_ = input;
		y2_ = y1_;
		y1_ = output;

		// カットオフを掃引すると稀に発振する。NaN を後段へ流さない
		if (!std::isfinite(output)) {
			Reset();
			return 0.0f;
		}
		return output;
	}

	// ───────────────────────────── Effects ─────────────────────────────

	float Distort(float input, float drive, float mix)
	{
		const float amount = (std::max)(drive, 0.01f);
		const float wet = std::tanh(input * amount) / std::tanh(amount);
		return input + (wet - input) * std::clamp(mix, 0.0f, 1.0f);
	}

	void RemoveDCOffset(std::vector<float>& samples, int channels, int sampleRate)
	{
		if (samples.empty() || channels <= 0 || sampleRate <= 0) { return; }

		// y[n] = x[n] - x[n-1] + R * y[n-1]。R が 1 に近いほど遮断周波数が下がる
		constexpr float kCutoffHz = 20.0f;
		const float r = 1.0f - (kTwoPi * kCutoffHz / static_cast<float>(sampleRate));

		const size_t frames = samples.size() / static_cast<size_t>(channels);
		for (int ch = 0; ch < channels; ++ch) {
			float previousInput = 0.0f;
			float previousOutput = 0.0f;
			for (size_t f = 0; f < frames; ++f) {
				const size_t index = f * channels + ch;
				const float input = samples[index];
				const float output = input - previousInput + r * previousOutput;
				previousInput = input;
				previousOutput = output;
				samples[index] = output;
			}
		}
	}

	void ApplyDelay(std::vector<float>& samples, int channels, int sampleRate,
		float delaySeconds, float feedback, float mix)
	{
		if (samples.empty() || channels <= 0 || sampleRate <= 0) { return; }
		if (delaySeconds <= 0.0f || mix <= 0.0f) { return; }

		const size_t delayFrames = static_cast<size_t>(delaySeconds * static_cast<float>(sampleRate));
		if (delayFrames == 0) { return; }

		const size_t frames = samples.size() / static_cast<size_t>(channels);
		// 1 以上のフィードバックは減衰しないので 0.95 で止める
		const float fb = std::clamp(feedback, 0.0f, 0.95f);
		const float wetAmount = std::clamp(mix, 0.0f, 1.0f);

		std::vector<float> line(delayFrames * static_cast<size_t>(channels), 0.0f);
		size_t writeIndex = 0;

		for (size_t f = 0; f < frames; ++f) {
			for (int ch = 0; ch < channels; ++ch) {
				const size_t sampleIndex = f * channels + ch;
				const size_t lineIndex = writeIndex * channels + ch;

				const float delayed = line[lineIndex];
				const float dry = samples[sampleIndex];
				line[lineIndex] = dry + delayed * fb;
				samples[sampleIndex] = dry + delayed * wetAmount;
			}
			writeIndex = (writeIndex + 1) % delayFrames;
		}
	}

	void ApplyReverb(std::vector<float>& samples, int channels, int sampleRate,
		float roomSize, float damping, float width, float mix)
	{
		if (samples.empty() || channels <= 0 || sampleRate <= 0) { return; }
		if (mix <= 0.0f) { return; }

		// Freeverb の標準チューニング（44100Hz 前提の遅延長）。
		// サンプルレートが違う場合は比で伸縮する
		static constexpr int kCombTuning[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
		static constexpr int kAllpassTuning[4] = { 556, 441, 341, 225 };
		static constexpr int kStereoSpread = 23;

		const float rateScale = static_cast<float>(sampleRate) / 44100.0f;
		const float feedback = std::clamp(roomSize, 0.0f, 1.0f) * 0.28f + 0.70f;
		const float damp = std::clamp(damping, 0.0f, 1.0f) * 0.4f;
		const float wetAmount = std::clamp(mix, 0.0f, 1.0f);
		const float spread = std::clamp(width, 0.0f, 1.0f);

		struct Comb {
			std::vector<float> buffer;
			size_t index = 0;
			float filterStore = 0.0f;
		};
		struct Allpass {
			std::vector<float> buffer;
			size_t index = 0;
		};

		const int processChannels = (channels < 2) ? channels : 2;

		std::vector<std::array<Comb, 8>> combs(processChannels);
		std::vector<std::array<Allpass, 4>> allpasses(processChannels);

		for (int ch = 0; ch < processChannels; ++ch) {
			const size_t offset = (ch == 1) ? static_cast<size_t>(kStereoSpread * rateScale) : 0;
			for (int i = 0; i < 8; ++i) {
				const size_t length = static_cast<size_t>((std::max)(1.0f, kCombTuning[i] * rateScale)) + offset;
				combs[ch][i].buffer.assign(length, 0.0f);
			}
			for (int i = 0; i < 4; ++i) {
				const size_t length = static_cast<size_t>((std::max)(1.0f, kAllpassTuning[i] * rateScale)) + offset;
				allpasses[ch][i].buffer.assign(length, 0.0f);
			}
		}

		const size_t frames = samples.size() / static_cast<size_t>(channels);
		for (size_t f = 0; f < frames; ++f) {
			for (int ch = 0; ch < processChannels; ++ch) {
				const size_t sampleIndex = f * channels + ch;
				const float dry = samples[sampleIndex];
				// コムを8本並列に足すと振幅が跳ねるので入力を絞っておく（Freeverb と同じ係数）
				const float input = dry * 0.015f;

				float wet = 0.0f;
				for (Comb& comb : combs[ch]) {
					const float stored = comb.buffer[comb.index];
					comb.filterStore = stored * (1.0f - damp) + comb.filterStore * damp;
					comb.buffer[comb.index] = input + comb.filterStore * feedback;
					comb.index = (comb.index + 1) % comb.buffer.size();
					wet += stored;
				}
				for (Allpass& allpass : allpasses[ch]) {
					const float stored = allpass.buffer[allpass.index];
					const float output = -wet + stored;
					allpass.buffer[allpass.index] = wet + stored * 0.5f;
					allpass.index = (allpass.index + 1) % allpass.buffer.size();
					wet = output;
				}

				samples[sampleIndex] = dry + wet * wetAmount;
			}
		}

		// ステレオのとき、広がりの指定に応じて左右を混ぜ戻す
		if (processChannels == 2 && spread < 1.0f) {
			for (size_t f = 0; f < frames; ++f) {
				float& left = samples[f * channels + 0];
				float& right = samples[f * channels + 1];
				const float mid = (left + right) * 0.5f;
				left = mid + (left - mid) * spread;
				right = mid + (right - mid) * spread;
			}
		}
	}

	// ───────────────────────────── FFT ─────────────────────────────

	std::vector<float> ComputeSpectrum(const float* input, size_t count, size_t fftSize)
	{
		// 2のべき乗へ切り下げる
		size_t size = 1;
		while (size * 2 <= fftSize) { size *= 2; }
		if (size < 4 || !input || count == 0) { return {}; }

		std::vector<std::complex<float>> data(size, { 0.0f, 0.0f });
		const size_t copyCount = (count < size) ? count : size;
		for (size_t i = 0; i < copyCount; ++i) {
			// ハン窓。窓を掛けないと両端の段差が全帯域へ漏れる
			const float window = 0.5f - 0.5f * std::cos(kTwoPi * static_cast<float>(i) / static_cast<float>(size - 1));
			data[i] = { input[i] * window, 0.0f };
		}

		// ビット反転による並べ替え
		for (size_t i = 1, j = 0; i < size; ++i) {
			size_t bit = size >> 1;
			for (; j & bit; bit >>= 1) { j ^= bit; }
			j ^= bit;
			if (i < j) { std::swap(data[i], data[j]); }
		}

		// Cooley-Tukey
		for (size_t length = 2; length <= size; length <<= 1) {
			const float angle = -kTwoPi / static_cast<float>(length);
			const std::complex<float> step(std::cos(angle), std::sin(angle));
			for (size_t i = 0; i < size; i += length) {
				std::complex<float> w(1.0f, 0.0f);
				for (size_t k = 0; k < length / 2; ++k) {
					const std::complex<float> even = data[i + k];
					const std::complex<float> odd = data[i + k + length / 2] * w;
					data[i + k] = even + odd;
					data[i + k + length / 2] = even - odd;
					w *= step;
				}
			}
		}

		std::vector<float> magnitudes(size / 2);
		const float scale = 2.0f / static_cast<float>(size);
		for (size_t i = 0; i < magnitudes.size(); ++i) {
			magnitudes[i] = std::abs(data[i]) * scale;
		}
		return magnitudes;
	}

} // namespace SoundDSP
