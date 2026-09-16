#pragma once
#include <string>
#include <vector>

#include "SoundDSP.h"

/// <summary>
/// 音源1本ぶんの設定（設計書 Phase 5 の SoundLayer）。
///
/// 1レイヤー＝「1つの発振器＋そのピッチ／音量の時間変化＋専用フィルタ」。
/// 実際のゲームSEは1波形では作りにくいので、これを複数重ねて1つのSEにする。
/// </summary>
struct SELayer
{
	std::string name = "Layer";
	bool enabled = true;

	// ── 音源（Phase 2）──
	SoundDSP::WaveType wave = SoundDSP::WaveType::Sine;
	/// <summary>ピッチエンベロープが無効なときの周波数。ノイズでは粒の粗さになる</summary>
	float frequency = 440.0f;
	/// <summary>矩形波のデューティ比。0.5 で対称</summary>
	float pulseWidth = 0.5f;
	/// <summary>FM の変調比（搬送波に対する倍率）。0 で FM 無効</summary>
	float fmRatio = 0.0f;
	/// <summary>FM の深さ。金属質な響き（キィン）を作るのに効く</summary>
	float fmAmount = 0.0f;
	/// <summary>ノイズの乱数シード。変えると同じ設定でも違う「当たり」になる</summary>
	int noiseSeed = 12345;

	// ── 時間（Phase 3）──
	/// <summary>このレイヤーの長さ（秒）</summary>
	float duration = 0.25f;
	/// <summary>SE の先頭から何秒遅れて鳴り出すか。層をずらして厚みを出す</summary>
	float startDelay = 0.0f;

	// ── 音量とパン（Phase 1/3）──
	float volume = 0.8f;
	/// <summary>-1 で左、+1 で右</summary>
	float pan = 0.0f;
	SoundDSP::Envelope envelope{};

	// ── ピッチエンベロープ（Phase 3）──
	bool pitchSweepEnabled = false;
	float pitchStart = 2000.0f;
	float pitchEnd = 500.0f;
	/// <summary>掃引に掛ける秒数。0 ならレイヤー全体を使う</summary>
	float pitchSweepTime = 0.0f;
	SoundDSP::SweepCurve pitchCurve = SoundDSP::SweepCurve::Exponential;

	// ── レイヤー専用フィルタ（Phase 4）──
	SoundDSP::FilterType filterType = SoundDSP::FilterType::None;
	float filterCutoff = 2000.0f;
	float filterResonance = 0.707f;
	/// <summary>カットオフも掃引するか。true なら cutoff → cutoffEnd へ動く</summary>
	bool filterSweepEnabled = false;
	float filterCutoffEnd = 500.0f;
};

/// <summary>歪み（Phase 4.4）</summary>
struct SEDistortion
{
	bool enabled = false;
	float drive = 4.0f;
	float mix = 1.0f;
};

/// <summary>ディレイ（Phase 4.5）</summary>
struct SEDelay
{
	bool enabled = false;
	float time = 0.12f;
	float feedback = 0.35f;
	float mix = 0.4f;
};

/// <summary>残響（Phase 4.6）</summary>
struct SEReverb
{
	bool enabled = false;
	float roomSize = 0.5f;
	float damping = 0.5f;
	float width = 1.0f;
	float mix = 0.3f;
};

/// <summary>
/// SE 1つぶんの定義。これがそのまま Resource/Sounds/&lt;name&gt;.sound になる（設計書 Phase 6）。
///
/// レイヤーを混ぜたあと、マスター側で 歪み → ディレイ → リバーブ → フィルタ の順に掛かる。
/// </summary>
struct SoundDefinition
{
	std::string name = "NewSound";
	int sampleRate = 44100;
	float masterVolume = 1.0f;
	/// <summary>書き出す前にピークを normalizePeak へ揃えるか</summary>
	bool normalize = true;
	float normalizePeak = 0.95f;
	/// <summary>ゲームから同時に鳴った音を捌く優先度（設計書 Phase 8）。大きいほど優先</summary>
	int priority = 50;

	std::vector<SELayer> layers;

	SEDistortion distortion{};
	SEDelay delay{};
	SEReverb reverb{};

	SoundDSP::FilterType masterFilterType = SoundDSP::FilterType::None;
	float masterFilterCutoff = 8000.0f;
	float masterFilterResonance = 0.707f;

	/// <summary>レイヤーの遅延と長さから決まる、エフェクト前の音の長さ（秒）</summary>
	float GetLayerDuration() const;
	/// <summary>エフェクトの尾を含めた最終的な長さ（秒）</summary>
	float GetTotalDuration() const;
};

/// <summary>
/// Resource/Sounds/&lt;name&gt;.sound の読み書き（設計書 Phase 6）。
///
/// 中身は JSON。VFXFile と違って生の json では持たず構造体へ写しているのは、
/// 合成側がフィールドを直接触るため（パラメータが増えたらここも足すこと）。
/// </summary>
namespace SoundFile {

	/// <summary>拡張子を含むパス。Resource/Sounds/&lt;name&gt;.sound</summary>
	std::string MakeFilePath(const std::string& soundName);

	/// <summary>ファイルがあるか</summary>
	bool Exists(const std::string& soundName);

	/// <summary>読み込む。ファイルが無い・壊れている場合は false（起動は止めない）</summary>
	bool Load(const std::string& soundName, SoundDefinition& outDefinition);

	/// <summary>書き出す。ディレクトリが無ければ作る</summary>
	bool Save(const SoundDefinition& definition);

	/// <summary>削除する</summary>
	bool Delete(const std::string& soundName);

	/// <summary>Resource/Sounds/ にある .sound の名前一覧（拡張子なし・名前順）</summary>
	std::vector<std::string> ListNames();

} // namespace SoundFile
