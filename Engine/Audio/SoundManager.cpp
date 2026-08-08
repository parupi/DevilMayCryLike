#include "SoundManager.h"
#include "Audio/Audio.h"
#include "Debugger/GlobalVariables.h"
#include "Utility/DeltaTime.h"
#include "Utility/Logger.h"

#include <algorithm>
#include <filesystem>

namespace {
	// 音量設定の保存先（Resource/GlobalVariables/Sound/SoundVolume.json）
	const char* kVolumeDirectory = "Sound";
	const char* kVolumeGroup = "SoundVolume";
	const char* kKeyMaster = "MasterVolume";
	const char* kKeyBGM = "BGMVolume";
	const char* kKeySE = "SEVolume";

	// Audio::SoundLoadWave が内部で組み立てるパスと同じもの。
	// 開けなかった場合に向こうは assert で落ちるので、こちらで先に有無を見る
	std::filesystem::path MakeSoundPath(const std::string& name) {
		return std::filesystem::path("Resource/sound") / (name + ".wav");
	}
}

SoundManager& SoundManager::GetInstance() {
	static SoundManager instance;
	return instance;
}

void SoundManager::Initialize() {
	GlobalVariables* global = &GlobalVariables::GetInstance();
	// 保存済みの設定があれば読み込む（ファイルが無ければ既定値のまま）
	global->LoadFile(kVolumeDirectory, kVolumeGroup);
	global->AddItem(kVolumeGroup, kKeyMaster, masterVolume_);
	global->AddItem(kVolumeGroup, kKeyBGM, bgmVolume_);
	global->AddItem(kVolumeGroup, kKeySE, seVolume_);
	PullVolumes();

	// 攻撃SEは戦闘中に何度も鳴るので最初に載せておく。
	// BGM は1曲で数十MBあるため、必要になるシーンの初期化で Preload する
	Preload("SwordSlash");
	Preload("SwordHit");
}

void SoundManager::Update() {
	// エディタや GlobalVariables ウィンドウで触られた音量を毎フレーム拾う
	PullVolumes();

#ifdef _DEBUG
	// エディタで再生を止めていても音のフェードは実時間で進めたい
	const float deltaTime = DeltaTime::GetUnscaledDeltaTime();
#else
	const float deltaTime = DeltaTime::GetDeltaTime();
#endif // _DEBUG

	UpdateChannel(current_, deltaTime);
	UpdateChannel(previous_, deltaTime);

	// 消えきった曲は解放してボイスを返す
	if (previous_.IsActive() && previous_.gain <= 0.0f && previous_.targetGain <= 0.0f) {
		ReleaseChannel(previous_);
	}
}

void SoundManager::Finalize() {
	ReleaseChannel(current_);
	ReleaseChannel(previous_);
	paused_ = false;
}

void SoundManager::Preload(const std::string& name) {
	if (name.empty()) { return; }

	Audio& audio = Audio::GetInstance();
	if (audio.GetSoundDataMap().count(name) != 0) { return; }

	std::error_code ec;
	if (!std::filesystem::exists(MakeSoundPath(name), ec)) {
		Logger::Log("[SoundManager] 音声ファイルが見つかりません: " + name + "\n");
		return;
	}

	audio.SoundLoadWave(name.c_str());
}

void SoundManager::PlayBGM(const std::string& name, float fadeTime) {
	if (name.empty()) { return; }

	// 曲を指定して鳴らす＝止めておく理由はもう無い。
	// （メニューから直接タイトルへ戻った場合など、ポーズしたまま次のシーンへ抜ける経路がある）
	ResumeBGM();

	// 同じ曲なら鳴らし直さない。ステートの Update から毎フレーム呼ばれても平気なようにしておく
	if (current_.IsActive() && current_.name == name) {
		current_.targetGain = 1.0f;
		if (current_.gain < 1.0f && current_.fadeSpeed <= 0.0f) {
			current_.fadeSpeed = (fadeTime > 0.0f) ? (1.0f / fadeTime) : 0.0f;
		}
		return;
	}

	// 読み込みは再生の前に済ませる（未読込だと SoundPlayWave が -1 を返す）
	Preload(name);

	// 今の曲はフェードアウトさせながら残す
	FadeOutCurrent(fadeTime);

	current_.name = name;
	current_.handle = Audio::GetInstance().SoundPlayWave(name.c_str(), true);
	if (!current_.IsActive()) {
		// 読み込めていない、もしくは空きボイスが無い
		current_ = BGMChannel{};
		return;
	}

	current_.gain = (fadeTime > 0.0f) ? 0.0f : 1.0f;
	current_.targetGain = 1.0f;
	current_.fadeSpeed = (fadeTime > 0.0f) ? (1.0f / fadeTime) : 0.0f;
	ApplyGain(current_);
}

void SoundManager::StopBGM(float fadeTime) {
	FadeOutCurrent(fadeTime);
}

void SoundManager::PauseBGM() {
	if (paused_) { return; }
	paused_ = true;
	if (current_.IsActive()) { Audio::GetInstance().PauseBGM(current_.handle); }
	if (previous_.IsActive()) { Audio::GetInstance().PauseBGM(previous_.handle); }
}

void SoundManager::ResumeBGM() {
	if (!paused_) { return; }
	paused_ = false;
	if (current_.IsActive()) { Audio::GetInstance().ReStartBGM(current_.handle); }
	if (previous_.IsActive()) { Audio::GetInstance().ReStartBGM(previous_.handle); }
}

void SoundManager::PlaySE(const std::string& name, float volume) {
	if (name.empty()) { return; }

	Preload(name);

	const int handle = Audio::GetInstance().SoundPlayWave(name.c_str(), false);
	if (handle < 0) { return; }

	Audio::GetInstance().SetBGMVolume(handle, std::clamp(volume, 0.0f, 1.0f) * seVolume_ * masterVolume_);
}

void SoundManager::SetMasterVolume(float volume) {
	masterVolume_ = std::clamp(volume, 0.0f, 1.0f);
	GlobalVariables::GetInstance().SetValue(kVolumeGroup, kKeyMaster, masterVolume_);
}

void SoundManager::SetBGMVolume(float volume) {
	bgmVolume_ = std::clamp(volume, 0.0f, 1.0f);
	GlobalVariables::GetInstance().SetValue(kVolumeGroup, kKeyBGM, bgmVolume_);
}

void SoundManager::SetSEVolume(float volume) {
	seVolume_ = std::clamp(volume, 0.0f, 1.0f);
	GlobalVariables::GetInstance().SetValue(kVolumeGroup, kKeySE, seVolume_);
}

void SoundManager::SaveVolumes() {
	GlobalVariables::GetInstance().SaveFile(kVolumeDirectory, kVolumeGroup);
}

void SoundManager::UpdateChannel(BGMChannel& channel, float deltaTime) {
	if (!channel.IsActive()) { return; }

	if (channel.gain != channel.targetGain) {
		if (channel.fadeSpeed <= 0.0f) {
			channel.gain = channel.targetGain;
		} else {
			// 行き過ぎないよう、進む向き側を目標値で頭打ちにする
			// （std::min / std::max は Windows.h のマクロと衝突するのでここでは使わない）
			const float step = channel.fadeSpeed * deltaTime;
			if (channel.gain < channel.targetGain) {
				channel.gain = std::clamp(channel.gain + step, 0.0f, channel.targetGain);
			} else {
				channel.gain = std::clamp(channel.gain - step, channel.targetGain, 1.0f);
			}
		}
	}

	// 音量設定はエディタから動かせるので、フェード中でなくても毎フレーム掛け直す
	ApplyGain(channel);
}

void SoundManager::ApplyGain(const BGMChannel& channel) const {
	if (!channel.IsActive()) { return; }
	Audio::GetInstance().SetBGMVolume(channel.handle, channel.gain * bgmVolume_ * masterVolume_);
}

void SoundManager::ReleaseChannel(BGMChannel& channel) {
	if (channel.IsActive()) {
		Audio::GetInstance().StopBGM(channel.handle);
	}
	channel = BGMChannel{};
}

void SoundManager::FadeOutCurrent(float fadeTime) {
	if (!current_.IsActive()) {
		current_ = BGMChannel{};
		return;
	}

	// 退避先が埋まっている（前の入れ替えがまだ終わっていない）なら、そちらは諦めて止める
	if (previous_.IsActive()) {
		ReleaseChannel(previous_);
	}

	previous_ = current_;
	previous_.targetGain = 0.0f;
	previous_.fadeSpeed = (fadeTime > 0.0f) ? (1.0f / fadeTime) : 0.0f;
	current_ = BGMChannel{};

	// フェード無し指定ならその場で止める
	if (previous_.fadeSpeed <= 0.0f) {
		ReleaseChannel(previous_);
	}
}

void SoundManager::PullVolumes() {
	GlobalVariables* global = &GlobalVariables::GetInstance();
	// Initialize 前に呼ばれた場合は既定値のまま
	if (!global->HasItem(kVolumeGroup, kKeyMaster)) { return; }

	masterVolume_ = std::clamp(global->GetValueRef<float>(kVolumeGroup, kKeyMaster), 0.0f, 1.0f);
	bgmVolume_ = std::clamp(global->GetValueRef<float>(kVolumeGroup, kKeyBGM), 0.0f, 1.0f);
	seVolume_ = std::clamp(global->GetValueRef<float>(kVolumeGroup, kKeySE), 0.0f, 1.0f);
}
