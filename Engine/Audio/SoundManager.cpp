#include "SoundManager.h"
#include "Audio/Audio.h"
#include "Audio/SE/SoundAssetLibrary.h"
#include "Debugger/GlobalVariables.h"
#include "Utility/DeltaTime.h"
#include "Utility/Logger.h"

#include <algorithm>
#include <cmath>
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

	// 音速（m/s）。ドップラーの計算にだけ使う
	constexpr float kSpeedOfSound = 340.0f;
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

	// 鳴り終わった SE を一覧から外す。優先度の判定がここの中身を見る
	PruneSEVoices();
}

void SoundManager::Finalize() {
	ReleaseChannel(current_);
	ReleaseChannel(previous_);

	for (const SEVoice& voice : seVoices_) {
		Audio::GetInstance().StopBGM(voice.handle);
	}
	seVoices_.clear();
	// 焼いた波形の登録も忘れる。Audio::Finalize がバッファごと捨てるので、
	// ここを残すと「登録済みのつもりで鳴らない」状態になる
	SoundAssets::InvalidateAll();

	paused_ = false;
}

void SoundManager::Preload(const std::string& name) {
	if (name.empty()) { return; }

	Audio& audio = Audio::GetInstance();
	if (audio.HasSound(name)) { return; }

	// SEエディタで作った .sound を先に見る。
	// 同名の .wav があっても、わざわざ作ったほうを鳴らしたいはずなのでこちらが勝つ
	if (SoundAssets::Exists(name)) {
		if (SoundAssets::Preload(name)) { return; }
		Logger::Log("[SoundManager] .sound の合成に失敗しました: " + name + "\n");
	}

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

int SoundManager::PlaySE(const std::string& name, float volume) {
	SEPlayParams params;
	params.name = name;
	params.volume = volume;
	return PlaySE(params);
}

int SoundManager::PlaySE(const SEPlayParams& params) {
	if (params.name.empty()) { return -1; }

	Preload(params.name);

	// 優先度の指定が無ければ .sound 側の値を使う（WAV しか無ければ既定値）
	int priority = params.priority;
	if (priority < 0) {
		priority = SoundAssets::Exists(params.name) ? SoundAssets::GetPriority(params.name) : 50;
	}

	PruneSEVoices();
	if (!MakeRoomForSE(priority)) {
		// 今鳴っている音のほうが重要。鳴らさずに諦める
		return -1;
	}

	Audio& audio = Audio::GetInstance();
	const int handle = audio.SoundPlayWave(params.name.c_str(), params.loop);
	if (handle < 0) { return -1; }

	audio.SetBGMVolume(handle, std::clamp(params.volume, 0.0f, 1.0f) * seVolume_ * masterVolume_);
	if (params.pitch != 1.0f) { audio.SetPitch(handle, params.pitch); }
	if (params.pan != 0.0f) { audio.SetPan(handle, params.pan); }

	seVoices_.push_back(SEVoice{ handle, params.name, priority, params.loop });
	return handle;
}

bool SoundManager::Compute3D(const Vector3& worldPosition, const Vector3& velocity,
	float& outAttenuation, float& outPan, float& outPitch) const {
	outAttenuation = 1.0f;
	outPan = 0.0f;
	outPitch = 1.0f;

	// 聞き手が渡されていなければ普通の SE として扱う
	if (!hasListener_) { return true; }

	const Vector3 toSource = worldPosition - listenerPosition_;
	const float distance = Length(toSource);

	// 遠すぎる音は鳴らさない
	if (distance >= seMaxDistance_) { return false; }

	// 距離減衰。逆二乗だと近づいた瞬間だけ極端に大きくなるので逆距離にする
	if (distance > seMinDistance_) {
		outAttenuation = seMinDistance_ / distance;
		// 打ち切り点で「ブツッ」と消えないよう、最後の1/4でフェードさせる
		const float fadeStart = seMaxDistance_ * 0.75f;
		if (distance > fadeStart) {
			outAttenuation *= (seMaxDistance_ - distance) / (seMaxDistance_ - fadeStart);
		}
	}
	outAttenuation = std::clamp(outAttenuation, 0.0f, 1.0f);

	if (distance > 0.001f) {
		const Vector3 direction = Normalize(toSource);
		// パンは聞き手の右方向との内積。真横で振り切らないよう 0.85 まで
		// （振り切ると片耳から完全に消えて不自然に聞こえる）
		outPan = std::clamp(Dot(direction, listenerRight_) * 0.85f, -1.0f, 1.0f);

		// ドップラー。音源と聞き手の、視線方向の速度差で再生速度を変える
		const float sourceSpeed = Dot(velocity, direction);
		const float listenerSpeed = Dot(listenerVelocity_, direction);
		const float denominator = kSpeedOfSound + sourceSpeed;
		if (std::fabs(denominator) > 1.0f) {
			outPitch = std::clamp((kSpeedOfSound + listenerSpeed) / denominator, 0.5f, 2.0f);
		}
	}
	return true;
}

int SoundManager::PlaySE3D(const std::string& name, const Vector3& worldPosition,
	float volume, const Vector3& velocity) {
	float attenuation = 1.0f;
	float pan = 0.0f;
	float pitch = 1.0f;
	// 遠すぎる音はボイスを取らない。優先度の取り合いにも参加させない
	if (!Compute3D(worldPosition, velocity, attenuation, pan, pitch)) { return -1; }

	SEPlayParams params;
	params.name = name;
	params.volume = volume * attenuation;
	params.pan = pan;
	params.pitch = pitch;
	return PlaySE(params);
}

bool SoundManager::UpdateSE3D(int handle, const Vector3& worldPosition, float volume) {
	if (handle < 0) { return false; }

	// 一覧に無い＝もう鳴り終わったか席を奪われた後。番号を使い回している別の音を触らない
	const auto it = std::find_if(seVoices_.begin(), seVoices_.end(),
		[handle](const SEVoice& voice) { return voice.handle == handle; });
	if (it == seVoices_.end()) { return false; }

	float attenuation = 1.0f;
	float pan = 0.0f;
	float pitch = 1.0f;
	// 範囲外へ出たら止めずに無音にする。戻ってきたらまた聞こえる
	if (!Compute3D(worldPosition, {}, attenuation, pan, pitch)) { attenuation = 0.0f; }

	Audio& audio = Audio::GetInstance();
	audio.SetBGMVolume(handle, std::clamp(volume * attenuation, 0.0f, 1.0f) * seVolume_ * masterVolume_);
	audio.SetPan(handle, pan);
	return true;
}

void SoundManager::StopSE(int handle) {
	if (handle < 0) { return; }

	// 一覧に無い＝もう鳴り終わったか、優先度の高い音に席を奪われた後。
	// そのまま Audio へ渡すと、同じ番号を使い回している別の音を止めてしまう
	const auto it = std::find_if(seVoices_.begin(), seVoices_.end(),
		[handle](const SEVoice& voice) { return voice.handle == handle; });
	if (it == seVoices_.end()) { return; }

	Audio::GetInstance().StopBGM(handle);
	seVoices_.erase(it);
}

void SoundManager::SetListener(const Vector3& position, const Vector3& forward, const Vector3& right,
	const Vector3& velocity) {
	hasListener_ = true;
	listenerPosition_ = position;
	listenerForward_ = forward;
	listenerRight_ = right;
	listenerVelocity_ = velocity;
}

void SoundManager::SetSEDistanceRange(float minDistance, float maxDistance) {
	seMinDistance_ = (std::max)(minDistance, 0.01f);
	seMaxDistance_ = (std::max)(maxDistance, seMinDistance_ + 0.01f);
}

void SoundManager::PruneSEVoices() {
	Audio& audio = Audio::GetInstance();
	seVoices_.erase(
		std::remove_if(seVoices_.begin(), seVoices_.end(),
			[&audio](const SEVoice& voice) { return !audio.IsPlaying(voice.handle); }),
		seVoices_.end());
}

bool SoundManager::MakeRoomForSE(int priority) {
	if (seVoices_.size() < kMaxSEVoices) { return true; }

	// いちばん優先度の低い音を探す。同点なら古いほう（先頭側）が犠牲になる
	auto lowest = seVoices_.begin();
	for (auto it = seVoices_.begin(); it != seVoices_.end(); ++it) {
		if (it->priority < lowest->priority) { lowest = it; }
	}

	// 自分のほうが低い（か同じ）なら割り込まない
	if (lowest->priority >= priority) { return false; }

	Audio::GetInstance().StopBGM(lowest->handle);
	seVoices_.erase(lowest);
	return true;
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
