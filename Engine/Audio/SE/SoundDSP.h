#pragma once
#include <cstdint>
#include <vector>

/// <summary>
/// SE 合成に使う最小限の DSP 部品（設計書 Phase 2〜4）。
///
/// ここにあるのは「1サンプルずつ進める素の処理」だけで、パラメータの持ち方や
/// 時間変化のさせ方は <see cref="SoundDefinition"/> と SoundSynth 側の仕事。
/// オフライン生成専用なので速度より読みやすさを優先している。
/// </summary>
namespace SoundDSP {

	// ───────────────────────────── 波形 ─────────────────────────────

	enum class WaveType {
		Sine,
		Square,
		Triangle,
		Saw,
		WhiteNoise,
		PinkNoise,
	};

	const char* ToString(WaveType type);
	WaveType WaveFromString(const char* name);
	/// <summary>ImGui のコンボ用。WaveType の並び順と一致している</summary>
	const char* const* WaveTypeNames();
	int WaveTypeCount();

	/// <summary>
	/// 単一波形の発振器。
	///
	/// ノイズも「周波数」を持つ点に注意。白色ノイズを素直に毎サンプル乱数で作ると
	/// ピッチスイープが効かず、設計書の Dodge（Noise → Pitch Down）が作れない。
	/// そこで **指定の周波数で乱数を引き直し、その間を補間する**（バリューノイズ）。
	/// 周波数を上げるほど白色ノイズに近づき、下げるほど低くこもった音になる。
	/// </summary>
	class Oscillator
	{
	public:
		void SetWaveType(WaveType type) { waveType_ = type; }
		WaveType GetWaveType() const { return waveType_; }

		void SetFrequency(float frequency) { frequency_ = frequency; }
		void SetSampleRate(float sampleRate) { sampleRate_ = sampleRate; }
		/// <summary>矩形波のデューティ比（0.5 で対称）。他の波形では無視される</summary>
		void SetPulseWidth(float width) { pulseWidth_ = width; }
		/// <summary>FM。ratio は搬送波に対する変調波の倍率、amount は変調の深さ</summary>
		void SetFM(float ratio, float amount) { fmRatio_ = ratio; fmAmount_ = amount; }

		void Reset(uint32_t seed = 12345u);

		/// <summary>1サンプル進めて値を返す</summary>
		float Next();
		/// <summary>設計書の Oscillator::Process 相当。周波数一定で埋める</summary>
		void Process(float* output, int sampleCount);

	private:
		float NextNoiseSample();

		WaveType waveType_ = WaveType::Sine;
		float sampleRate_ = 44100.0f;
		float frequency_ = 440.0f;
		float pulseWidth_ = 0.5f;
		float fmRatio_ = 0.0f;
		float fmAmount_ = 0.0f;

		float phase_ = 0.0f;   // 0〜1
		float fmPhase_ = 0.0f;

		// バリューノイズ用。phase_ でこの2つを補間する
		float heldNoise_ = 0.0f;
		float previousNoise_ = 0.0f;
		bool noiseStarted_ = false;
		uint32_t rngState_ = 12345u;
		// ピンクノイズ（Paul Kellet の近似）用の状態
		float pink_[7] = {};
	};

	// ───────────────────────────── 包絡線 ─────────────────────────────

	/// <summary>
	/// ADSR に Hold を足したもの（設計書 Phase 3）。
	/// SE では「立ち上がってすぐ落ちる」形が多く、ピークを一瞬保つ Hold があると作りやすい。
	///
	/// 各時間は秒。合計が音の長さを超える場合は比率を保ったまま縮める。
	/// </summary>
	struct Envelope
	{
		float attack = 0.005f;
		float hold = 0.0f;
		float decay = 0.10f;
		float sustain = 0.0f;   // 0〜1 のレベル。時間ではない
		float release = 0.05f;
		/// <summary>減衰の曲がり方。1 で直線、大きいほど最初に一気に落ちる</summary>
		float curve = 2.0f;

		/// <summary>時刻 t（秒）の音量。duration は音全体の長さ</summary>
		float Evaluate(float t, float duration) const;
	};

	/// <summary>時間変化の曲線（ピッチスイープとフィルタスイープで共用）</summary>
	enum class SweepCurve {
		Linear,
		Exponential, // 周波数の変化は指数のほうが耳に自然
		EaseOut,
		EaseIn,
	};

	const char* ToString(SweepCurve curve);
	SweepCurve SweepFromString(const char* name);
	const char* const* SweepCurveNames();
	int SweepCurveCount();

	/// <summary>0〜1 の進捗を曲線に通す</summary>
	float ApplySweepCurve(SweepCurve curve, float t01);
	/// <summary>始点から終点へ。Exponential のときだけ周波数比で補間する</summary>
	float SweepValue(float from, float to, float t01, SweepCurve curve);

	// ───────────────────────────── フィルタ ─────────────────────────────

	enum class FilterType {
		None,
		LowPass,
		HighPass,
		BandPass,
	};

	const char* ToString(FilterType type);
	FilterType FilterFromString(const char* name);
	const char* const* FilterTypeNames();
	int FilterTypeCount();

	/// <summary>RBJ クックブックの双2次フィルタ。LPF / HPF / BPF を切り替えて使う</summary>
	class Biquad
	{
	public:
		void Configure(FilterType type, float sampleRate, float cutoff, float q);
		void Reset();
		float Process(float input);

	private:
		float b0_ = 1.0f, b1_ = 0.0f, b2_ = 0.0f, a1_ = 0.0f, a2_ = 0.0f;
		float x1_ = 0.0f, x2_ = 0.0f, y1_ = 0.0f, y2_ = 0.0f;
		bool passthrough_ = true;
	};

	// ───────────────────────────── エフェクト ─────────────────────────────

	/// <summary>
	/// tanh による軟らかい歪み。drive を上げるほど倍音が増える。
	/// tanh(drive) で割り戻しているので、drive を変えても音量は大きく動かない
	/// </summary>
	float Distort(float input, float drive, float mix);

	/// <summary>
	/// 直流成分を取り除く（1次のハイパスを 20Hz 付近に置く）。
	///
	/// デューティ比をずらした矩形波や、低域に寄せたノイズは波形が上下どちらかへ偏る。
	/// 偏ったぶんは音として聞こえないのに振幅だけ食うので、正規化すると
	/// そのぶん本体が小さくなるし、再生開始・終了でスピーカーがボコッと鳴る
	/// </summary>
	void RemoveDCOffset(std::vector<float>& samples, int channels, int sampleRate);

	/// <summary>ディレイ（山彦）。インターリーブされたバッファへその場で掛ける</summary>
	void ApplyDelay(std::vector<float>& samples, int channels, int sampleRate,
		float delaySeconds, float feedback, float mix);

	/// <summary>
	/// Freeverb 系の残響（コムフィルタ8本 + オールパス4本）。
	/// ステレオのときは左右で遅延長をずらして広がりを出す
	/// </summary>
	void ApplyReverb(std::vector<float>& samples, int channels, int sampleRate,
		float roomSize, float damping, float width, float mix);

	// ───────────────────────────── 解析 ─────────────────────────────

	/// <summary>
	/// 実数入力の振幅スペクトル。エディタの表示用なので精度より手軽さ優先。
	/// fftSize は2のべき乗（それ以外は切り下げる）。戻り値は fftSize/2 個
	/// </summary>
	std::vector<float> ComputeSpectrum(const float* input, size_t count, size_t fftSize);

} // namespace SoundDSP
