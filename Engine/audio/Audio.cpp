#include "Audio.h"
#include <cassert>
#include <algorithm>


Audio& Audio::GetInstance() {
	static Audio instance;
	return instance;
}

void Audio::Initialize() {
	HRESULT result;
	// インスタンスの生成
	result = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	// マスターボイスの生成
	result = xAudio2->CreateMasteringVoice(&masterVoice);

	// SetPan の出力行列はマスター側のチャンネル数ぶん要る
	if (masterVoice) {
		XAUDIO2_VOICE_DETAILS details{};
		masterVoice->GetVoiceDetails(&details);
		masterChannels_ = static_cast<int>(details.InputChannels);
	}
}
void Audio::StopBGM(int resourceNum) {
	if (!IsValidVoice(resourceNum)) { return; }
	pSourceVoices_[resourceNum]->Stop();
	pSourceVoices_[resourceNum]->FlushSourceBuffers();
}

void Audio::PauseBGM(int resourceNum) {
	if (!IsValidVoice(resourceNum)) { return; }
	pSourceVoices_[resourceNum]->Stop();
}

void Audio::ReStartBGM(int resourceNum) {
	if (!IsValidVoice(resourceNum)) { return; }
	pSourceVoices_[resourceNum]->Start();
}

void Audio::SetBGMVolume(int resourceNum, float volume) {
	if (!IsValidVoice(resourceNum)) { return; }
	pSourceVoices_[resourceNum]->SetVolume(std::clamp(volume, 0.0f, 1.0f));
}

void Audio::SetPitch(int resourceNum, float ratio) {
	if (!IsValidVoice(resourceNum)) { return; }
	// XAUDIO2_MAX_FREQ_RATIO を超えると SetFrequencyRatio が失敗する。
	// 既定の上限は 2.0 なので、生成時の指定なしでも通る範囲に収める
	pSourceVoices_[resourceNum]->SetFrequencyRatio(std::clamp(ratio, 0.25f, 2.0f));
}

void Audio::SetPan(int resourceNum, float pan) {
	if (!IsValidVoice(resourceNum)) { return; }

	const int sourceChannels = voiceChannels_[resourceNum];
	if (sourceChannels <= 0 || masterChannels_ <= 0) { return; }
	// ステレオ出力以外（モノ / 5.1ch など）は素直な行列が作れないので触らない
	if (masterChannels_ != 2) { return; }

	const float value = std::clamp(pan, -1.0f, 1.0f);
	const float left = (value <= 0.0f) ? 1.0f : (1.0f - value);
	const float right = (value >= 0.0f) ? 1.0f : (1.0f + value);

	// 行列は [出力ch数 × 入力ch数]。入力chごとに左右への配分を並べる
	if (sourceChannels == 1) {
		float matrix[2] = { left, right };
		pSourceVoices_[resourceNum]->SetOutputMatrix(nullptr, 1, 2, matrix);
	} else if (sourceChannels == 2) {
		// ステレオ音源はもとの左右を保ったまま、片側を絞る形にする
		float matrix[4] = { left, 0.0f, 0.0f, right };
		pSourceVoices_[resourceNum]->SetOutputMatrix(nullptr, 2, 2, matrix);
	}
}

bool Audio::IsPlaying(int resourceNum) const {
	if (!IsValidVoice(resourceNum)) { return false; }
	XAUDIO2_VOICE_STATE state{};
	pSourceVoices_[resourceNum]->GetState(&state);
	return state.BuffersQueued > 0;
}

void Audio::RegisterGeneratedSound(const std::string& name, const std::vector<BYTE>& pcm,
	int channels, int sampleRate) {
	if (name.empty() || pcm.empty() || channels <= 0 || sampleRate <= 0) { return; }

	// 作り直す前に、古いバッファを指しているボイスを止める。
	// ここを飛ばすと XAudio2 が解放済みのメモリを読みに行く
	StopVoicesUsing(name);

	WAVEFORMATEX wfex{};
	wfex.wFormatTag = WAVE_FORMAT_PCM;
	wfex.nChannels = static_cast<WORD>(channels);
	wfex.nSamplesPerSec = static_cast<DWORD>(sampleRate);
	wfex.wBitsPerSample = 16;
	wfex.nBlockAlign = static_cast<WORD>(channels * 16 / 8);
	wfex.nAvgBytesPerSec = wfex.nSamplesPerSec * wfex.nBlockAlign;
	wfex.cbSize = 0;

	SoundData soundData{};
	soundData.wfex = wfex;
	soundData.pBuffer = pcm;
	soundData.bufferSize = static_cast<unsigned int>(pcm.size());
	soundData.playSoundLength = static_cast<int>(pcm.size() / wfex.nBlockAlign);

	soundDataMap[name] = std::move(soundData);
}

void Audio::StopVoicesUsing(const std::string& name) {
	for (size_t i = 0; i < kMaxPlayWave; i++) {
		if (pSourceVoices_[i] == nullptr || voiceSoundNames_[i] != name) { continue; }
		pSourceVoices_[i]->Stop();
		pSourceVoices_[i]->FlushSourceBuffers();
		// ボイス自体は残しておく（次の SoundPlayWave で作り直される）が、
		// 参照していた名前は消して二重に止めないようにする
		voiceSoundNames_[i].clear();
	}
}

const std::string& Audio::GetVoiceSoundName(int resourceNum) const {
	static const std::string kEmpty;
	if (resourceNum < 0 || resourceNum >= static_cast<int>(kMaxPlayWave)) { return kEmpty; }
	return voiceSoundNames_[resourceNum];
}

void Audio::SoundLoadWave(const char* filename) {
	if (soundDataMap.count(filename)) {
		// キーが存在する場合、処理を中断
		return;
	}

	// ファイル入力streamのインスタンス
	std::ifstream file;
	// 今回はサウンドのディレクトリが1つのため
	std::string directory = "resource/sound/";
	directory += filename;
	directory += ".wav";
	file.open(directory, std::ios_base::binary);
	assert(file.is_open());

	// .wavデータ読み込み
	// RIFFヘッダーの読み込み
	RiffHeader riff;
	file.read((char*)&riff, sizeof(riff));

	// ファイルがRIFFかチェック
	if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
		assert(0);
	}
	// タイプがWAVEがチェック
	if (strncmp(riff.type, "WAVE", 4) != 0) {
		assert(0);
	}
	// Formatチャンクの読み込み
	FormatChunk format = {};
	// チャンクヘッダーの確認
	file.read((char*)&format, sizeof(ChunkHeader));
	if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
		assert(0);
	}
	// チャンク本体の読み込み
	assert(format.chunk.size <= sizeof(format.fmt));
	file.read((char*)&format.fmt, format.chunk.size);

	// Dataチャンクの読み込み
	ChunkHeader data;
	file.read((char*)&data, sizeof(data));
	// JUNKチャンクを検出した場合
	if (strncmp(data.id, "JUNK", 4) == 0) {
		// 読み取り位置をJUNKチャンクの終わりまで進める
		file.seekg(data.size, std::ios_base::cur);
		// 再読み込み
		file.read((char*)&data, sizeof(data));
	}
	// LISTチャンクを検出した場合
	if (strncmp(data.id, "LIST", 4) == 0) {
		// 読み取り位置をLISTチャンクの終わりまで進める
		file.seekg(data.size, std::ios_base::cur);
		// 再読み込み
		file.read((char*)&data, sizeof(data));
	}
	// INFOISFTチャンクを検出した場合
	if (strncmp(data.id, "INFOISFT", 8) == 0) {
		// 読み取り位置をINFOISFTチャンクの終わりまで進める
		file.seekg(data.size, std::ios_base::cur);
		// 再読み込み
		file.read((char*)&data, sizeof(data));
	}

	if (strncmp(data.id, "data", 4) != 0) {
		assert(0);
	}

	// Dataチャンクのデータ部 (波形のデータ) の読み込み
	SoundData soundData = {};
	soundData.pBuffer.resize(data.size);
	file.read(reinterpret_cast<char*>(soundData.pBuffer.data()), data.size);

	// ファイルクローズ
	file.close();

	soundData.wfex = format.fmt;
	soundData.bufferSize = data.size;
	soundData.playSoundLength = data.size / format.fmt.nBlockAlign;

	soundDataMap[filename] = std::move(soundData);
}

void Audio::SoundUnload(const char* filename) {
	SoundData* soundData = &soundDataMap[filename];
	soundData->pBuffer.clear();
	soundData->bufferSize = 0;
	soundData->wfex = {};
}

int Audio::SoundPlayWave(const char* filename, const bool isLoop) {
	HRESULT result;

	// 読み込んでいない音を operator[] で引くと空の波形フォーマットが出来てしまい、
	// そのまま CreateSourceVoice に渡して落ちる。見つからなければ鳴らさずに抜ける
	auto itSound = soundDataMap.find(filename);
	if (itSound == soundDataMap.end()) { return -1; }
	SoundData& soundData = itSound->second;

	// 今回使うサウンドデータ
	int sourceNum = -1;

	// 使用できるリソースを検索
	sourceNum = SearchSourceVoice(pSourceVoices_.data());

	// 使用できるリソースがない場合は-1を返す
	if (sourceNum == -1) { return -1; }

	// 再生停止中、もしくは残りの再生数が最小のリソースを使用。
	// 下で作り直すので、古いボイスはここで破棄しておく
	// （破棄せずに上書きすると再生のたびに IXAudio2SourceVoice が漏れる）
	if (pSourceVoices_[sourceNum] != nullptr) {
		pSourceVoices_[sourceNum]->Stop();
		pSourceVoices_[sourceNum]->FlushSourceBuffers();
		pSourceVoices_[sourceNum]->DestroyVoice();
		pSourceVoices_[sourceNum] = nullptr;
	}

	// 波形フォーマットをもとにSourceVoiceの生成。
	// 毎回作り直しているので、前の再生で設定したピッチや定位はここで初期値へ戻る
	result = xAudio2->CreateSourceVoice(&pSourceVoices_[sourceNum], &soundData.wfex);
	assert(SUCCEEDED(result));

	// どの波形を鳴らしているかを控えておく（StopVoicesUsing と SetPan が見る）
	voiceSoundNames_[sourceNum] = filename;
	voiceChannels_[sourceNum] = static_cast<int>(soundData.wfex.nChannels);

	// 再生する波形データの設定
	XAUDIO2_BUFFER buf = SetBuffer(isLoop, soundData);

	// 波形データの再生
	result = pSourceVoices_[sourceNum]->SubmitSourceBuffer(&buf);
	result = pSourceVoices_[sourceNum]->Start();

	return sourceNum;
}

void Audio::Finalize() {
	// BGMリソースの解放
	for (auto SourceVoice : pSourceVoices_) {
		if (SourceVoice != nullptr) {
			SourceVoice->DestroyVoice();
		}
	}
	pSourceVoices_.fill(nullptr);
	for (std::string& name : voiceSoundNames_) { name.clear(); }
	voiceChannels_.fill(0);
	masterVoice->DestroyVoice();
	xAudio2.Reset();
	soundDataMap.clear();
}

int Audio::SearchSourceVoice(IXAudio2SourceVoice** sourceVoices) {
	// 今回再生するリソース
	int sourceVoiceNum = -1;

	// リソースのバッファ
	unsigned int soundBuffer = 0;

	// 使用できる再生リソースを検索
	for (int i = 0; i < kMaxPlayWave; i++) {
		if (sourceVoices[i] == nullptr) {
			sourceVoiceNum = i;
			break;
		}
		// 現在の状態を取得
		XAUDIO2_VOICE_STATE state;
		sourceVoices[i]->GetState(&state);

		// バッファが0ならば再生可能と判断
		if (state.BuffersQueued == 0) {
			sourceVoiceNum = i;
			break;
		} else {
			// 初期値もしくはバッファが最小の場合は入れ替え
			if (soundBuffer == 0 || soundBuffer > state.BuffersQueued) {
				soundBuffer = state.BuffersQueued;
				sourceVoiceNum = i;
			}
		}
	}

	return sourceVoiceNum;
}

bool Audio::IsValidVoice(int resourceNum) const {
	if (resourceNum < 0 || resourceNum >= static_cast<int>(kMaxPlayWave)) { return false; }
	return pSourceVoices_[resourceNum] != nullptr;
}

XAUDIO2_BUFFER Audio::SetBuffer(bool loop, const SoundData& sound) {
	// バッファ設定
	XAUDIO2_BUFFER buffer;

	// バッファの初期化
	memset(&buffer, 0x00, sizeof(buffer));
	buffer.pAudioData = sound.pBuffer.data();
	buffer.AudioBytes = sound.bufferSize;
	buffer.PlayBegin = 0;
	buffer.PlayLength = sound.playSoundLength;

	// ループ設定
	if (loop) {
		buffer.LoopBegin = 0;
		buffer.LoopLength = sound.playSoundLength;
		buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	return buffer;
}
