#include "SoundDefinition.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace {

	const std::string kDirectory = "Resource/Sounds/";
	const std::string kExtension = ".sound";

	// json から enum を読む。未知の名前は既定値へ倒す（手書きで壊しても落ちないように）
	SoundDSP::WaveType ReadWave(const nlohmann::json& parent, const char* key, SoundDSP::WaveType fallback)
	{
		if (!parent.contains(key) || !parent[key].is_string()) { return fallback; }
		return SoundDSP::WaveFromString(parent[key].get<std::string>().c_str());
	}

	SoundDSP::FilterType ReadFilter(const nlohmann::json& parent, const char* key, SoundDSP::FilterType fallback)
	{
		if (!parent.contains(key) || !parent[key].is_string()) { return fallback; }
		return SoundDSP::FilterFromString(parent[key].get<std::string>().c_str());
	}

	SoundDSP::SweepCurve ReadSweep(const nlohmann::json& parent, const char* key, SoundDSP::SweepCurve fallback)
	{
		if (!parent.contains(key) || !parent[key].is_string()) { return fallback; }
		return SoundDSP::SweepFromString(parent[key].get<std::string>().c_str());
	}

	nlohmann::json WriteEnvelope(const SoundDSP::Envelope& envelope)
	{
		nlohmann::json json;
		json["Attack"] = envelope.attack;
		json["Hold"] = envelope.hold;
		json["Decay"] = envelope.decay;
		json["Sustain"] = envelope.sustain;
		json["Release"] = envelope.release;
		json["Curve"] = envelope.curve;
		return json;
	}

	SoundDSP::Envelope ReadEnvelope(const nlohmann::json& json)
	{
		SoundDSP::Envelope envelope;
		if (!json.is_object()) { return envelope; }
		envelope.attack = json.value("Attack", envelope.attack);
		envelope.hold = json.value("Hold", envelope.hold);
		envelope.decay = json.value("Decay", envelope.decay);
		envelope.sustain = json.value("Sustain", envelope.sustain);
		envelope.release = json.value("Release", envelope.release);
		envelope.curve = json.value("Curve", envelope.curve);
		return envelope;
	}

	nlohmann::json WriteLayer(const SELayer& layer)
	{
		nlohmann::json json;
		json["Name"] = layer.name;
		json["Enabled"] = layer.enabled;
		json["Wave"] = SoundDSP::ToString(layer.wave);
		json["Frequency"] = layer.frequency;
		json["PulseWidth"] = layer.pulseWidth;
		json["FMRatio"] = layer.fmRatio;
		json["FMAmount"] = layer.fmAmount;
		json["NoiseSeed"] = layer.noiseSeed;
		json["Duration"] = layer.duration;
		json["StartDelay"] = layer.startDelay;
		json["Volume"] = layer.volume;
		json["Pan"] = layer.pan;
		json["Envelope"] = WriteEnvelope(layer.envelope);

		nlohmann::json pitch;
		pitch["Enabled"] = layer.pitchSweepEnabled;
		pitch["Start"] = layer.pitchStart;
		pitch["End"] = layer.pitchEnd;
		pitch["Time"] = layer.pitchSweepTime;
		pitch["Curve"] = SoundDSP::ToString(layer.pitchCurve);
		json["PitchEnvelope"] = pitch;

		nlohmann::json filter;
		filter["Type"] = SoundDSP::ToString(layer.filterType);
		filter["Cutoff"] = layer.filterCutoff;
		filter["Resonance"] = layer.filterResonance;
		filter["Sweep"] = layer.filterSweepEnabled;
		filter["CutoffEnd"] = layer.filterCutoffEnd;
		json["Filter"] = filter;

		return json;
	}

	SELayer ReadLayer(const nlohmann::json& json)
	{
		SELayer layer;
		if (!json.is_object()) { return layer; }

		layer.name = json.value("Name", layer.name);
		layer.enabled = json.value("Enabled", layer.enabled);
		layer.wave = ReadWave(json, "Wave", layer.wave);
		layer.frequency = json.value("Frequency", layer.frequency);
		layer.pulseWidth = json.value("PulseWidth", layer.pulseWidth);
		layer.fmRatio = json.value("FMRatio", layer.fmRatio);
		layer.fmAmount = json.value("FMAmount", layer.fmAmount);
		layer.noiseSeed = json.value("NoiseSeed", layer.noiseSeed);
		layer.duration = json.value("Duration", layer.duration);
		layer.startDelay = json.value("StartDelay", layer.startDelay);
		layer.volume = json.value("Volume", layer.volume);
		layer.pan = json.value("Pan", layer.pan);

		if (json.contains("Envelope")) {
			layer.envelope = ReadEnvelope(json["Envelope"]);
		}

		if (json.contains("PitchEnvelope") && json["PitchEnvelope"].is_object()) {
			const auto& pitch = json["PitchEnvelope"];
			layer.pitchSweepEnabled = pitch.value("Enabled", layer.pitchSweepEnabled);
			layer.pitchStart = pitch.value("Start", layer.pitchStart);
			layer.pitchEnd = pitch.value("End", layer.pitchEnd);
			layer.pitchSweepTime = pitch.value("Time", layer.pitchSweepTime);
			layer.pitchCurve = ReadSweep(pitch, "Curve", layer.pitchCurve);
		}

		if (json.contains("Filter") && json["Filter"].is_object()) {
			const auto& filter = json["Filter"];
			layer.filterType = ReadFilter(filter, "Type", layer.filterType);
			layer.filterCutoff = filter.value("Cutoff", layer.filterCutoff);
			layer.filterResonance = filter.value("Resonance", layer.filterResonance);
			layer.filterSweepEnabled = filter.value("Sweep", layer.filterSweepEnabled);
			layer.filterCutoffEnd = filter.value("CutoffEnd", layer.filterCutoffEnd);
		}

		return layer;
	}

} // namespace

float SoundDefinition::GetLayerDuration() const
{
	float longest = 0.0f;
	for (const SELayer& layer : layers) {
		if (!layer.enabled) { continue; }
		const float end = (std::max)(layer.startDelay, 0.0f) + (std::max)(layer.duration, 0.0f);
		if (end > longest) { longest = end; }
	}
	return longest;
}

float SoundDefinition::GetTotalDuration() const
{
	float total = GetLayerDuration();
	if (total <= 0.0f) { return 0.0f; }

	// エフェクトの尾が切れると「プツッ」と終わるので、鳴り終わるぶんを足しておく
	if (delay.enabled && delay.time > 0.0f) {
		// フィードバックが強いほど尾が長い。-60dB まで落ちるおおよその回数
		const float feedback = std::clamp(delay.feedback, 0.0f, 0.95f);
		const float repeats = (feedback > 0.01f) ? std::clamp(std::log(0.001f) / std::log(feedback), 1.0f, 12.0f) : 1.0f;
		total += delay.time * repeats;
	}
	if (reverb.enabled && reverb.mix > 0.0f) {
		total += 0.6f + reverb.roomSize * 1.8f;
	}

	// 生成時間とメモリの歯止め。10秒を超える SE はまず使わない
	return (std::min)(total, 10.0f);
}

namespace SoundFile {

	std::string MakeFilePath(const std::string& soundName)
	{
		return kDirectory + soundName + kExtension;
	}

	bool Exists(const std::string& soundName)
	{
		if (soundName.empty()) { return false; }
		std::error_code ec;
		return std::filesystem::exists(MakeFilePath(soundName), ec);
	}

	bool Load(const std::string& soundName, SoundDefinition& outDefinition)
	{
		std::ifstream file(MakeFilePath(soundName));
		if (!file.is_open()) { return false; }

		nlohmann::json root;
		try {
			file >> root;
		} catch (const nlohmann::json::exception&) {
			// 壊れたファイルで起動を止めない。呼び出し側が false を見て WAV へ落とす
			return false;
		}
		if (!root.is_object()) { return false; }

		SoundDefinition definition;
		definition.name = root.value("Name", soundName);
		definition.sampleRate = root.value("SampleRate", definition.sampleRate);
		definition.masterVolume = root.value("MasterVolume", definition.masterVolume);
		definition.normalize = root.value("Normalize", definition.normalize);
		definition.normalizePeak = root.value("NormalizePeak", definition.normalizePeak);
		definition.priority = root.value("Priority", definition.priority);

		if (root.contains("Layers") && root["Layers"].is_array()) {
			for (const auto& layerJson : root["Layers"]) {
				definition.layers.push_back(ReadLayer(layerJson));
			}
		}

		if (root.contains("Distortion") && root["Distortion"].is_object()) {
			const auto& json = root["Distortion"];
			definition.distortion.enabled = json.value("Enabled", definition.distortion.enabled);
			definition.distortion.drive = json.value("Drive", definition.distortion.drive);
			definition.distortion.mix = json.value("Mix", definition.distortion.mix);
		}
		if (root.contains("Delay") && root["Delay"].is_object()) {
			const auto& json = root["Delay"];
			definition.delay.enabled = json.value("Enabled", definition.delay.enabled);
			definition.delay.time = json.value("Time", definition.delay.time);
			definition.delay.feedback = json.value("Feedback", definition.delay.feedback);
			definition.delay.mix = json.value("Mix", definition.delay.mix);
		}
		if (root.contains("Reverb") && root["Reverb"].is_object()) {
			const auto& json = root["Reverb"];
			definition.reverb.enabled = json.value("Enabled", definition.reverb.enabled);
			definition.reverb.roomSize = json.value("RoomSize", definition.reverb.roomSize);
			definition.reverb.damping = json.value("Damping", definition.reverb.damping);
			definition.reverb.width = json.value("Width", definition.reverb.width);
			definition.reverb.mix = json.value("Mix", definition.reverb.mix);
		}
		if (root.contains("MasterFilter") && root["MasterFilter"].is_object()) {
			const auto& json = root["MasterFilter"];
			definition.masterFilterType = ReadFilter(json, "Type", definition.masterFilterType);
			definition.masterFilterCutoff = json.value("Cutoff", definition.masterFilterCutoff);
			definition.masterFilterResonance = json.value("Resonance", definition.masterFilterResonance);
		}

		outDefinition = std::move(definition);
		return true;
	}

	bool Save(const SoundDefinition& definition)
	{
		if (definition.name.empty()) { return false; }

		std::error_code ec;
		std::filesystem::create_directories(kDirectory, ec);

		nlohmann::json root;
		root["Name"] = definition.name;
		root["SampleRate"] = definition.sampleRate;
		root["MasterVolume"] = definition.masterVolume;
		root["Normalize"] = definition.normalize;
		root["NormalizePeak"] = definition.normalizePeak;
		root["Priority"] = definition.priority;

		nlohmann::json layers = nlohmann::json::array();
		for (const SELayer& layer : definition.layers) {
			layers.push_back(WriteLayer(layer));
		}
		root["Layers"] = layers;

		nlohmann::json distortion;
		distortion["Enabled"] = definition.distortion.enabled;
		distortion["Drive"] = definition.distortion.drive;
		distortion["Mix"] = definition.distortion.mix;
		root["Distortion"] = distortion;

		nlohmann::json delay;
		delay["Enabled"] = definition.delay.enabled;
		delay["Time"] = definition.delay.time;
		delay["Feedback"] = definition.delay.feedback;
		delay["Mix"] = definition.delay.mix;
		root["Delay"] = delay;

		nlohmann::json reverb;
		reverb["Enabled"] = definition.reverb.enabled;
		reverb["RoomSize"] = definition.reverb.roomSize;
		reverb["Damping"] = definition.reverb.damping;
		reverb["Width"] = definition.reverb.width;
		reverb["Mix"] = definition.reverb.mix;
		root["Reverb"] = reverb;

		nlohmann::json masterFilter;
		masterFilter["Type"] = SoundDSP::ToString(definition.masterFilterType);
		masterFilter["Cutoff"] = definition.masterFilterCutoff;
		masterFilter["Resonance"] = definition.masterFilterResonance;
		root["MasterFilter"] = masterFilter;

		std::ofstream file(MakeFilePath(definition.name));
		if (!file.is_open()) { return false; }
		file << root.dump(4);
		return file.good();
	}

	bool Delete(const std::string& soundName)
	{
		if (soundName.empty()) { return false; }
		std::error_code ec;
		return std::filesystem::remove(MakeFilePath(soundName), ec);
	}

	std::vector<std::string> ListNames()
	{
		std::vector<std::string> names;

		std::error_code ec;
		if (!std::filesystem::is_directory(kDirectory, ec)) { return names; }

		for (const auto& entry : std::filesystem::directory_iterator(kDirectory, ec)) {
			if (!entry.is_regular_file()) { continue; }
			const std::filesystem::path& path = entry.path();
			if (path.extension() != kExtension) { continue; }
			names.push_back(path.stem().string());
		}
		std::sort(names.begin(), names.end());
		return names;
	}

} // namespace SoundFile
