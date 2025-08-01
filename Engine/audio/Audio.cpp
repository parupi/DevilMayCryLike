#include "Audio.h"
#include <cassert>
#include <algorithm>

Audio* Audio::instance = nullptr;
std::once_flag Audio::initInstanceFlag;

Audio* Audio::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new Audio;
		});
	return instance;
}

void Audio::Initialize()
{
	HRESULT result;
	// インスタンスの生成
	result = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	// マスターボイスの生成
	result = xAudio2->CreateMasteringVoice(&masterVoice);

}
void Audio::StopBGM(int resourceNum)
{
	pSourceVoices_[resourceNum]->Stop();
	pSourceVoices_[resourceNum]->FlushSourceBuffers();
}

void Audio::PauseBGM(int resourceNum)
{
	pSourceVoices_[resourceNum]->Stop();
}

void Audio::ReStartBGM(int resourceNum)
{
	pSourceVoices_[resourceNum]->Start();
}

void Audio::SetBGMVolume(int resourceNum, float volume)
{
	pSourceVoices_[resourceNum]->SetVolume(std::clamp(volume, 0.0f, 1.0f));
}

void Audio::SoundLoadWave(const char* filename)
{
	//HRESULT result;
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
	char* pBuffer = new char[data.size];
	file.read(pBuffer, data.size);

	// ファイルクローズ
	file.close();

	// SoundDataの生成
	SoundData soundData = {};

	soundData.wfex = format.fmt;
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
	soundData.bufferSize = data.size;
	soundData.playSoundLength = data.size / format.fmt.nBlockAlign;

	soundDataMap[filename] = soundData;

	// 読み込んだ音声データをreturn
	//return soundData;
}

void Audio::SoundUnload(const char* filename)
{
	SoundData* soundData = &soundDataMap[filename];
	// バッファのメモリを解放
	delete[] soundData->pBuffer;

	soundData->pBuffer = 0;
	soundData->bufferSize = 0;
	soundData->wfex = {};
}

int Audio::SoundPlayWave(const char* filename, const bool isLoop)
{
	HRESULT result;

	SoundData& soundData = soundDataMap[filename];

	// 今回使うサウンドデータ
	int sourceNum = -1;

	// 使用できるリソースを検索
	sourceNum = SearchSourceVoice(pSourceVoices_.data());

	// 使用できるリソースがない場合は-1を返す
	if (sourceNum == -1) { return -1; }

	// 再生停止中、もしくは残りの再生数が最小のリソースを使用
	if(pSourceVoices_[sourceNum] != nullptr)
	{
		pSourceVoices_[sourceNum]->Stop();
		pSourceVoices_[sourceNum]->FlushSourceBuffers();
	}

	// 波形フォーマットをもとにSourceVoiceの生成
	result = xAudio2->CreateSourceVoice(&pSourceVoices_[sourceNum], &soundData.wfex);
	assert(SUCCEEDED(result));

	// 再生する波形データの設定
	XAUDIO2_BUFFER buf = SetBuffer(isLoop, soundData);

	// 波形データの再生
	result = pSourceVoices_[sourceNum]->SubmitSourceBuffer(&buf);
	result = pSourceVoices_[sourceNum]->Start();

	return sourceNum;
}

void Audio::Finalize()
{
	// BGMリソースの解放
	for (auto SourceVoice : pSourceVoices_)
	{
		if(SourceVoice != nullptr)
		{
			SourceVoice->DestroyVoice();
		}
	}
	masterVoice->DestroyVoice();
	xAudio2.Reset();
	delete instance;
	instance = nullptr;
}

int Audio::SearchSourceVoice(IXAudio2SourceVoice** sourceVoices)
{
	// 今回再生するリソース
	int sourceVoiceNum = -1;

	// リソースのバッファ
	unsigned int soundBuffer = 0;

	// 使用できる再生リソースを検索
	for (int i = 0; i < kMaxPlayWave; i++)
	{
		if (sourceVoices[i] == nullptr) {
			sourceVoiceNum = i;
			break;
		}
		// 現在の状態を取得
		XAUDIO2_VOICE_STATE state;
		sourceVoices[i]->GetState(&state);

		// バッファが0ならば再生可能と判断
		if (state.BuffersQueued == 0)
		{
			sourceVoiceNum = i;
			break;
		}
		else
		{
			// 初期値もしくはバッファが最小の場合は入れ替え
			if (soundBuffer == 0 || soundBuffer > state.BuffersQueued)
			{
				soundBuffer = state.BuffersQueued;
				sourceVoiceNum = i;
			}
		}
	}

	return sourceVoiceNum;
}

XAUDIO2_BUFFER Audio::SetBuffer(bool loop, const SoundData& sound)
{
	// バッファ設定
	XAUDIO2_BUFFER buffer;

	// バッファの初期化
	memset(&buffer, 0x00, sizeof(buffer));
	buffer.pAudioData = sound.pBuffer;
	buffer.AudioBytes = sound.bufferSize;
	buffer.PlayBegin = 0;
	buffer.PlayLength = sound.playSoundLength;

	// ループ設定
	if (loop)
	{
		buffer.LoopBegin = 0;
		buffer.LoopLength = sound.playSoundLength;
		buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	return buffer;
}

