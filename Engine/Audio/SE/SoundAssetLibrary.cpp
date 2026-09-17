#include "SoundAssetLibrary.h"

#include <unordered_map>

#include "Audio/Audio.h"
#include "SoundSynthesizer.h"
#include "Utility/Logger.h"

namespace {

	// 焼き済みの .sound。波形そのものは Audio が持つので、ここには付随情報だけ置く
	struct RegisteredSound {
		int priority = 50;
	};

	std::unordered_map<std::string, RegisteredSound> g_registered;

} // namespace

namespace SoundAssets {

	std::vector<std::string> ListNames()
	{
		return SoundFile::ListNames();
	}

	bool Exists(const std::string& name)
	{
		return SoundFile::Exists(name);
	}

	bool Preload(const std::string& name)
	{
		if (name.empty()) { return false; }

		// 既に焼いてある。Audio 側にも残っていればそのまま使える
		const auto it = g_registered.find(name);
		if (it != g_registered.end() && Audio::GetInstance().HasSound(name)) {
			return true;
		}

		SoundDefinition definition;
		if (!SoundFile::Load(name, definition)) {
			return false;
		}

		const AudioBuffer buffer = SoundSynth::Render(definition);
		if (buffer.IsEmpty()) {
			Logger::Log("[SoundAssets] 合成結果が空でした: " + name + "\n");
			return false;
		}

		Audio::GetInstance().RegisterGeneratedSound(
			name, buffer.ToPCM16(), buffer.channels, buffer.sampleRate);

		g_registered[name] = RegisteredSound{ definition.priority };
		return true;
	}

	int GetPriority(const std::string& name)
	{
		const auto it = g_registered.find(name);
		if (it != g_registered.end()) { return it->second.priority; }

		// まだ焼いていない場合はファイルを覗く。ここで焼くと再生前に固まるので読むだけ
		SoundDefinition definition;
		if (SoundFile::Load(name, definition)) { return definition.priority; }
		return SoundDefinition{}.priority;
	}

	AudioBuffer RenderAndRegister(const SoundDefinition& definition, const std::string& registerName)
	{
		const std::string& name = registerName.empty() ? definition.name : registerName;

		AudioBuffer buffer = SoundSynth::Render(definition);
		if (name.empty() || buffer.IsEmpty()) { return buffer; }

		Audio::GetInstance().RegisterGeneratedSound(
			name, buffer.ToPCM16(), buffer.channels, buffer.sampleRate);

		g_registered[name] = RegisteredSound{ definition.priority };
		return buffer;
	}

	void Invalidate(const std::string& name)
	{
		g_registered.erase(name);
		// Audio 側の波形は残したままにする。鳴っている最中に消すと解放済みを読むため。
		// 次の Preload が RegisterGeneratedSound で作り直す（そのとき中で止まる）
	}

	void InvalidateAll()
	{
		g_registered.clear();
	}

} // namespace SoundAssets
