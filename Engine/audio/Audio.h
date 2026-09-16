#pragma once
#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib")

#include <fstream>
#include <wrl.h>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <array>
#include <vector>

// 音源の同時再生数
static const size_t kMaxPlayWave = 100;

class Audio
{
	Audio() = default;
	Audio(const Audio&) = delete;
	Audio& operator=(const Audio&) = delete;
public:

	// シングルトンインスタンスの取得
	static Audio& GetInstance();
private: // 構造体
	// チャンクヘッダー
	struct ChunkHeader {
		char id[4]; // チャンクごとのID
		int32_t size; // チャンクサイズ
	};

	// RIFFヘッダチャンク
	struct RiffHeader {
		ChunkHeader chunk; // "RIFF"
		char type[4]; // "WAVE"
	};

	// FMTチャンク
	struct FormatChunk {
		ChunkHeader chunk; // "fmt"
		WAVEFORMATEX fmt; // 波形フォーマット
	};

	// 音声データ
	struct SoundData {
		WAVEFORMATEX wfex; // 波形フォーマット
		std::vector<BYTE> pBuffer; // バッファ
		unsigned int bufferSize; // バッファのサイズ
		int playSoundLength;
	};

public:
	// 初期化
	void Initialize();

	/**
	 * @brief 音源の停止
	 * @param resourceNum BGMのリソース番号
	 */
	void StopBGM(int resourceNum);

	/**
	 * @brief 音源のポーズ
	 * @param resourceNum BGMのリソース番号
	 */
	void PauseBGM(int resourceNum);

	/**
	 * @brief 音源の再開
	 * @param resourceNum BGMのリソース番号
	 */
	void ReStartBGM(int resourceNum);

	/**
	 * @brief 音量調整
	 * @param resourceNum BGMのリソース番号
	 */
	void SetBGMVolume(int resourceNum, float volume);

	/**
	 * @brief 再生速度（＝ピッチ）を変える
	 * @param resourceNum BGMのリソース番号
	 * @param ratio 1.0 で原音。2.0 で1オクターブ上、0.5 で1オクターブ下
	 */
	void SetPitch(int resourceNum, float ratio);

	/**
	 * @brief 定位を変える
	 * @param resourceNum BGMのリソース番号
	 * @param pan -1.0 で左、0.0 で中央、+1.0 で右
	 */
	void SetPan(int resourceNum, float pan);

	/// @brief そのボイスがまだ鳴っているか
	bool IsPlaying(int resourceNum) const;

	// 音声読み込み
	void SoundLoadWave(const char* filename);
	// 音声データ解放
	void SoundUnload(const char* filename);

	/**
	 * @brief その名前の波形がメモリにあるか
	 */
	bool HasSound(const std::string& name) const { return soundDataMap.count(name) != 0; }

	/**
	 * @brief 生成した波形を名前付きで登録する。
	 *
	 * SEエディタで焼いた音を .wav へ書き出さずにそのまま鳴らすための入口。
	 * 登録後は SoundPlayWave(name) がファイル由来の音と同じように扱える。
	 *
	 * 同じ名前が既にあれば**中身を作り直す**（SoundLoadWave と違って早期 return しない）。
	 * このとき古いバッファを参照しているボイスは内部で止める。
	 *
	 * @param name 拡張子もフォルダも無い素の名前
	 * @param pcm 16bit PCM のバイト列
	 * @param channels チャンネル数
	 * @param sampleRate サンプリング周波数
	 */
	void RegisterGeneratedSound(const std::string& name, const std::vector<BYTE>& pcm,
		int channels, int sampleRate);

	/// @brief その波形を鳴らしているボイスを全部止める（バッファを作り直す前に呼ぶ）
	void StopVoicesUsing(const std::string& name);

	/**
	 * @brief そのボイスが今どの波形を鳴らしているか。
	 *
	 * ボイスのスロットは使い回されるので、再生番号だけ持っていると
	 * 「止めたつもりが別の音を止めていた」が起きる。止める前にここで確かめる
	 */
	const std::string& GetVoiceSoundName(int resourceNum) const;
	/**
	 * @brief 音源の再生
	 * @param soundData 音源データ
	 * @param isLoop ループするか　default : false
	 * @return int BGMのリソース番号
	 */
	int SoundPlayWave(const char* filename, const bool isLoop = false);
	// 終了
	void Finalize();

private:
	// 利用可能なソースボイスを検索
	int SearchSourceVoice(IXAudio2SourceVoice** sourceVoices);

	// 再生リソース番号が有効か。SoundPlayWave が失敗して返す -1 や、
	// まだ生成していないスロットをそのまま使うと落ちるので必ずここを通す
	bool IsValidVoice(int resourceNum) const;

	XAUDIO2_BUFFER SetBuffer(bool loop, const SoundData& sound);

private:
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masterVoice;

	std::unordered_map<std::string, Audio::SoundData> soundDataMap;

	std::array<IXAudio2SourceVoice*, kMaxPlayWave> pSourceVoices_ = { nullptr };
	// そのスロットが今どの波形を鳴らしているか。
	// 生成した波形を作り直すときに「その波形を参照しているボイス」を止めるために要る
	// （止めずに vector を作り直すと、XAudio2 が解放済みのメモリを読みに行く）
	std::array<std::string, kMaxPlayWave> voiceSoundNames_;
	// SetPan の出力行列はソースのチャンネル数ぶん必要なので覚えておく
	std::array<int, kMaxPlayWave> voiceChannels_ = {};

	// マスターボイスのチャンネル数。SetPan の行列サイズに使う
	int masterChannels_ = 2;

public:
	auto GetSoundData() { return soundDataMap; }
	// エディタ用。GetSoundData() は波形バッファごとコピーするので、
	// 毎フレーム一覧を舐めるようなところではこちらを使うこと
	const auto& GetSoundDataMap() const { return soundDataMap; }
};

