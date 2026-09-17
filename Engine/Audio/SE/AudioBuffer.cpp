#include "AudioBuffer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>

float AudioBuffer::DurationSeconds() const
{
	if (sampleRate <= 0) { return 0.0f; }
	return static_cast<float>(FrameCount()) / static_cast<float>(sampleRate);
}

void AudioBuffer::Resize(size_t frameCount)
{
	if (channels <= 0) { channels = 1; }
	samples.assign(frameCount * static_cast<size_t>(channels), 0.0f);
}

void AudioBuffer::Clear()
{
	samples.clear();
}

float AudioBuffer::Sample(size_t frame, int channel) const
{
	if (channel < 0 || channel >= channels) { return 0.0f; }
	const size_t index = frame * static_cast<size_t>(channels) + static_cast<size_t>(channel);
	if (index >= samples.size()) { return 0.0f; }
	return samples[index];
}

float AudioBuffer::PeakLevel() const
{
	float peak = 0.0f;
	for (float value : samples) {
		const float magnitude = std::fabs(value);
		if (magnitude > peak) { peak = magnitude; }
	}
	return peak;
}

float AudioBuffer::RmsLevel() const
{
	if (samples.empty()) { return 0.0f; }
	double total = 0.0;
	for (float value : samples) {
		total += static_cast<double>(value) * value;
	}
	return static_cast<float>(std::sqrt(total / static_cast<double>(samples.size())));
}

void AudioBuffer::NormalizeTo(float targetPeak)
{
	const float peak = PeakLevel();
	// 完全な無音を割ると inf になる。閾値は 16bit の 1LSB より十分下
	if (peak <= 1.0e-6f) { return; }

	const float gain = targetPeak / peak;
	for (float& value : samples) {
		value *= gain;
	}
}

void AudioBuffer::ClipToRange()
{
	for (float& value : samples) {
		value = std::clamp(value, -1.0f, 1.0f);
	}
}

void AudioBuffer::ApplyEdgeFade(float seconds)
{
	const size_t frames = FrameCount();
	if (frames == 0 || seconds <= 0.0f) { return; }

	size_t fadeFrames = static_cast<size_t>(seconds * static_cast<float>(sampleRate));
	// 短い音を潰さないよう、フェードは全体の 1/4 までに留める
	fadeFrames = std::clamp<size_t>(fadeFrames, 1, frames / 4 + 1);

	for (size_t i = 0; i < fadeFrames && i < frames; ++i) {
		const float gain = static_cast<float>(i) / static_cast<float>(fadeFrames);
		for (int ch = 0; ch < channels; ++ch) {
			samples[i * channels + ch] *= gain;
		}
	}
	for (size_t i = 0; i < fadeFrames && i < frames; ++i) {
		const float gain = static_cast<float>(i) / static_cast<float>(fadeFrames);
		const size_t frame = frames - 1 - i;
		for (int ch = 0; ch < channels; ++ch) {
			samples[frame * channels + ch] *= gain;
		}
	}
}

std::vector<uint8_t> AudioBuffer::ToPCM16() const
{
	std::vector<uint8_t> bytes(samples.size() * sizeof(int16_t));
	auto* out = reinterpret_cast<int16_t*>(bytes.data());

	for (size_t i = 0; i < samples.size(); ++i) {
		const float clamped = std::clamp(samples[i], -1.0f, 1.0f);
		// 32767 を掛けると +1.0 がぴったり最大値になり、-1.0 側は 1LSB 余る。
		// 非対称だが折り返しが起きないのでこちらを使う
		out[i] = static_cast<int16_t>(std::lround(clamped * 32767.0f));
	}
	return bytes;
}

bool AudioBuffer::WriteWavFile(const std::string& filePath) const
{
	if (samples.empty()) { return false; }

	std::error_code ec;
	const std::filesystem::path path(filePath);
	if (path.has_parent_path()) {
		std::filesystem::create_directories(path.parent_path(), ec);
	}

	std::ofstream file(path, std::ios::binary);
	if (!file.is_open()) { return false; }

	const std::vector<uint8_t> pcm = ToPCM16();
	const uint16_t bitsPerSample = 16;
	const uint16_t blockAlign = static_cast<uint16_t>(channels * bitsPerSample / 8);
	const uint32_t byteRate = static_cast<uint32_t>(sampleRate) * blockAlign;
	const uint32_t dataSize = static_cast<uint32_t>(pcm.size());

	auto writeU32 = [&file](uint32_t value) { file.write(reinterpret_cast<const char*>(&value), 4); };
	auto writeU16 = [&file](uint16_t value) { file.write(reinterpret_cast<const char*>(&value), 2); };

	file.write("RIFF", 4);
	writeU32(36 + dataSize);
	file.write("WAVE", 4);

	file.write("fmt ", 4);
	writeU32(16);
	writeU16(1); // WAVE_FORMAT_PCM
	writeU16(static_cast<uint16_t>(channels));
	writeU32(static_cast<uint32_t>(sampleRate));
	writeU32(byteRate);
	writeU16(blockAlign);
	writeU16(bitsPerSample);

	file.write("data", 4);
	writeU32(dataSize);
	file.write(reinterpret_cast<const char*>(pcm.data()), dataSize);

	return file.good();
}
