#pragma once
#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib")

#include <fstream>
#include <wrl.h>
#include <mutex>
#include <unordered_map>
#include <array>

// 音源の同時再生数
static const size_t kMaxPlayWave = 100;

class Audio
{
	static Audio* instance;
	static std::once_flag initInstanceFlag;

	Audio() = default;
	~Audio() = default;
	Audio(Audio&) = default;
	Audio& operator=(Audio&) = default;
public:

	// シングルトンインスタンスの取得
	static Audio* GetInstance();
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
		BYTE* pBuffer; // バッファの先頭アドレス
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

	// 音声読み込み
	void SoundLoadWave(const char* filename);
	// 音声データ解放
	void SoundUnload(const char* filename);
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

	XAUDIO2_BUFFER SetBuffer(bool loop, const SoundData& sound);

private:
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masterVoice;

	std::unordered_map<std::string, Audio::SoundData> soundDataMap;

	std::array<IXAudio2SourceVoice*, kMaxPlayWave> pSourceVoices_ = { nullptr };

public:
	auto GetSoundData() { return soundDataMap; }
};

