#pragma once
#include <string>

/// <summary>
/// BGM と SE をゲーム側から扱いやすくするための層。
///
/// Audio は XAudio2 の薄いラッパで「再生してボイス番号を返す」までしか面倒を見ないので、
/// 　・今どの曲が鳴っているのか
/// 　・曲の入れ替え（フェードアウト＋フェードイン）
/// 　・マスター／BGM／SE の音量
/// といったところをここでまとめて持つ。
/// </summary>
class SoundManager
{
public:
	// フェード時間を省略したときの既定値（秒）
	static constexpr float kDefaultFadeTime = 1.0f;

	static SoundManager& GetInstance();

	/// <summary>音量設定の読み込みと、常時使う SE の先読みを行う</summary>
	void Initialize();

	/// <summary>
	/// フェードを進める。実時間で動かすので、ポーズ中でも毎フレーム呼ぶこと
	/// </summary>
	void Update();

	/// <summary>鳴っている音を止める。Audio::Finalize より前に呼ぶ</summary>
	void Finalize();

	/// <summary>
	/// 波形をメモリへ載せる。読み込み済み・ファイルが無い場合は何もしない。
	/// BGM は1曲で数十MBあるので、シーンの初期化（＝どうせ止まっている間）に通しておく
	/// </summary>
	void Preload(const std::string& name);

	/// <summary>
	/// BGM を切り替える。同じ曲が鳴っていれば何もしないので、毎フレーム呼んでよい
	/// </summary>
	/// <param name="name">拡張子なしのファイル名（例: "TitleBGM"）</param>
	/// <param name="fadeTime">入れ替えに掛ける秒数。0 なら即時</param>
	void PlayBGM(const std::string& name, float fadeTime = kDefaultFadeTime);

	/// <summary>BGM を止める</summary>
	void StopBGM(float fadeTime = kDefaultFadeTime);

	/// <summary>メニューなどで BGM を一時停止する</summary>
	void PauseBGM();
	/// <summary>一時停止した BGM を再開する</summary>
	void ResumeBGM();

	/// <summary>
	/// SE を鳴らす
	/// </summary>
	/// <param name="name">拡張子なしのファイル名（例: "SwordHit"）</param>
	/// <param name="volume">この音だけに掛かる倍率。SE 全体の音量とは別</param>
	void PlaySE(const std::string& name, float volume = 1.0f);

	/// <summary>今鳴っている BGM の名前。鳴っていなければ空</summary>
	const std::string& GetCurrentBGMName() const { return current_.name; }
	bool IsPaused() const { return paused_; }

	float GetMasterVolume() const { return masterVolume_; }
	float GetBGMVolume() const { return bgmVolume_; }
	float GetSEVolume() const { return seVolume_; }
	void SetMasterVolume(float volume);
	void SetBGMVolume(float volume);
	void SetSEVolume(float volume);
	/// <summary>エディタで触った音量をファイルへ書き出す</summary>
	void SaveVolumes();

private:
	SoundManager() = default;
	~SoundManager() = default;
	SoundManager(const SoundManager&) = delete;
	SoundManager& operator=(const SoundManager&) = delete;

	// 鳴らしている BGM ひとつぶん
	struct BGMChannel {
		int handle = -1;         // Audio が返す再生リソース番号
		std::string name;
		float gain = 0.0f;       // フェード係数 0〜1
		float targetGain = 0.0f;
		float fadeSpeed = 0.0f;  // 1秒あたりの gain の変化量。0 なら即時

		bool IsActive() const { return handle >= 0; }
	};

	// フェードを1フレーム進めて音量を掛け直す
	void UpdateChannel(BGMChannel& channel, float deltaTime);
	// 現在の音量設定をボイスへ反映する
	void ApplyGain(const BGMChannel& channel) const;
	// ボイスを止めてチャンネルを空にする
	void ReleaseChannel(BGMChannel& channel);
	// current_ を previous_ へ退避してフェードアウトさせる
	void FadeOutCurrent(float fadeTime);
	// GlobalVariables 側の値をメンバへ取り込む
	void PullVolumes();

	BGMChannel current_{};
	BGMChannel previous_{};  // クロスフェード中に消えていく前の曲

	float masterVolume_ = 1.0f;
	float bgmVolume_ = 0.6f;
	float seVolume_ = 0.8f;
	bool paused_ = false;
};
