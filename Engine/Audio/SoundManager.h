#pragma once
#include <string>
#include <vector>

#include "Math/Vector3.h"

/// <summary>
/// SE を鳴らすときの細かい指定。名前と音量だけでよければ PlaySE(name, volume) を使う
/// </summary>
struct SEPlayParams
{
	std::string name;
	/// <summary>この音だけに掛かる倍率。SE 全体の音量とは別</summary>
	float volume = 1.0f;
	/// <summary>再生速度。1.0 で原音、2.0 で1オクターブ上（0.25〜2.0 に丸められる）</summary>
	float pitch = 1.0f;
	/// <summary>-1 で左、+1 で右</summary>
	float pan = 0.0f;
	/// <summary>
	/// 同時発音が上限に達したときの取り合いに使う。大きいほど優先。
	/// -1 のままなら .sound に書かれた値（無ければ既定値）が使われる
	/// </summary>
	int priority = -1;
};

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
	/// SE を鳴らす。
	///
	/// 音の出どころは2つあり、**同じ名前なら .sound（SEエディタで作ったもの）を優先する**。
	/// 　Resource/Sounds/&lt;name&gt;.sound … エンジン内で合成した SE
	/// 　Resource/sound/&lt;name&gt;.wav     … 従来どおりの WAV 素材
	/// </summary>
	/// <param name="name">拡張子なしの名前（例: "SwordHit"）</param>
	/// <param name="volume">この音だけに掛かる倍率。SE 全体の音量とは別</param>
	/// <returns>Audio の再生リソース番号。鳴らせなかったら -1</returns>
	int PlaySE(const std::string& name, float volume = 1.0f);

	/// <summary>ピッチや定位まで指定して SE を鳴らす</summary>
	int PlaySE(const SEPlayParams& params);

	/// <summary>
	/// ワールド座標を指定して SE を鳴らす（設計書 Phase 8「3Dサウンド」）。
	///
	/// <see cref="SetListener"/> で聞き手の位置と向きを渡しておくこと。
	/// 渡していない場合は距離減衰もパンも掛からず、普通の SE として鳴る。
	///
	/// 反映されるのは 音量（距離減衰）・パン・ドップラー（velocity を渡したときだけ）。
	/// 距離に応じたリバーブの掛け分けは、焼いた波形を差し替える形になるため入れていない
	/// </summary>
	/// <param name="velocity">音源の速度（m/s）。ゼロならドップラーは掛からない</param>
	int PlaySE3D(const std::string& name, const Vector3& worldPosition,
		float volume = 1.0f, const Vector3& velocity = {});

	/// <summary>SE を止める。PlaySE が返した番号を渡す</summary>
	void StopSE(int handle);

	/// <summary>
	/// 聞き手（≒カメラ）の状態を毎フレーム渡す。3D SE を使うときだけ必要
	/// </summary>
	/// <param name="right">聞き手の右方向。パンの左右判定に使う</param>
	void SetListener(const Vector3& position, const Vector3& forward, const Vector3& right,
		const Vector3& velocity = {});

	/// <summary>距離減衰の範囲。min 以内は減衰なし、max を超えると鳴らさない</summary>
	void SetSEDistanceRange(float minDistance, float maxDistance);

	/// <summary>
	/// SE の波形をあらかじめメモリへ載せる。
	/// .sound は初回再生時に合成が走るので、シーンの初期化で通しておくと引っかからない
	/// </summary>
	void PreloadSE(const std::string& name) { Preload(name); }

	/// <summary>今鳴っている SE の本数（エディタ表示用）</summary>
	int GetActiveSECount() const { return static_cast<int>(seVoices_.size()); }

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
	// 鳴らしている SE ひとつぶん。優先度で取り合うために記録しておく
	struct SEVoice {
		int handle = -1;
		std::string name;
		int priority = 0;
	};

	// 同時に鳴らす SE の上限（設計書 Phase 8「優先度」）。
	// Audio のボイス数（kMaxPlayWave = 100）より十分少なくして、
	// BGM のぶんを SE で食い潰さないようにする
	static constexpr size_t kMaxSEVoices = 32;

	// 鳴り終わった SE を一覧から外す
	void PruneSEVoices();
	// 上限に達しているとき、priority より低い SE を止めて席を空ける。
	// 空けられなければ false（＝この音は鳴らさない）
	bool MakeRoomForSE(int priority);

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

	std::vector<SEVoice> seVoices_;

	// 聞き手。SetListener が呼ばれるまでは 3D SE も普通の SE として鳴らす
	bool hasListener_ = false;
	Vector3 listenerPosition_{};
	Vector3 listenerForward_{ 0.0f, 0.0f, 1.0f };
	Vector3 listenerRight_{ 1.0f, 0.0f, 0.0f };
	Vector3 listenerVelocity_{};
	float seMinDistance_ = 6.0f;
	float seMaxDistance_ = 80.0f;

	float masterVolume_ = 1.0f;
	float bgmVolume_ = 0.6f;
	float seVolume_ = 0.8f;
	bool paused_ = false;
};
