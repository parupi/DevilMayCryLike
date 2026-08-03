#include "ParticleCurves.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace {

	const std::string kDirectory = "Resource/Particle/";

	nlohmann::json CurveToJson(const Curve& curve)
	{
		nlohmann::json array = nlohmann::json::array();
		for (const CurveKey& key : curve.GetKeys()) {
			array.push_back({ { "t", key.time }, { "v", key.value } });
		}
		return array;
	}

	Curve CurveFromJson(const nlohmann::json& array)
	{
		std::vector<CurveKey> keys;
		if (array.is_array()) {
			for (const auto& element : array) {
				CurveKey key{};
				key.time = element.value("t", 0.0f);
				key.value = element.value("v", 1.0f);
				keys.push_back(key);
			}
		}
		Curve curve;
		curve.SetKeys(std::move(keys));
		return curve;
	}

	nlohmann::json GradientToJson(const Gradient& gradient)
	{
		nlohmann::json array = nlohmann::json::array();
		for (const GradientKey& key : gradient.GetKeys()) {
			array.push_back({
				{ "t", key.time },
				{ "c", { key.color.x, key.color.y, key.color.z, key.color.w } }
				});
		}
		return array;
	}

	Gradient GradientFromJson(const nlohmann::json& array)
	{
		std::vector<GradientKey> keys;
		if (array.is_array()) {
			for (const auto& element : array) {
				GradientKey key{};
				key.time = element.value("t", 0.0f);
				if (element.contains("c") && element["c"].is_array() && element["c"].size() >= 4) {
					key.color = Vector4{
						element["c"][0].get<float>(),
						element["c"][1].get<float>(),
						element["c"][2].get<float>(),
						element["c"][3].get<float>()
					};
				} else {
					key.color = Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
				}
				keys.push_back(key);
			}
		}
		Gradient gradient;
		gradient.SetKeys(std::move(keys));
		return gradient;
	}

} // namespace

std::string ParticleCurves::MakeFilePath(const std::string& groupName)
{
	return kDirectory + groupName + ".curve.json";
}

nlohmann::json ParticleCurves::ToJson() const
{
	nlohmann::json j;
	j["SizeCurve"] = CurveToJson(sizeCurve);
	j["AlphaCurve"] = CurveToJson(alphaCurve);
	j["ColorGradient"] = GradientToJson(colorGradient);
	return j;
}

void ParticleCurves::FromJson(const nlohmann::json& object)
{
	if (!object.is_object()) return;

	if (object.contains("SizeCurve")) {
		sizeCurve = CurveFromJson(object["SizeCurve"]);
	}
	if (object.contains("AlphaCurve")) {
		alphaCurve = CurveFromJson(object["AlphaCurve"]);
	}
	if (object.contains("ColorGradient")) {
		colorGradient = GradientFromJson(object["ColorGradient"]);
	}
}

void ParticleCurves::Save(const std::string& groupName) const
{
	namespace fs = std::filesystem;

	const fs::path filePath(MakeFilePath(groupName));
	const fs::path directory = filePath.parent_path();
	if (!directory.empty() && !fs::exists(directory)) {
		fs::create_directories(directory);
	}

	std::ofstream file(filePath);
	if (!file.is_open()) return;

	file << ToJson().dump(4);
}

bool ParticleCurves::Load(const std::string& groupName)
{
	std::ifstream file(MakeFilePath(groupName));
	// ファイルが無いのは異常ではない。カーブ未設定のグループとして従来挙動で動く
	if (!file.is_open()) return false;

	nlohmann::json j;
	try {
		file >> j;
	} catch (const nlohmann::json::exception&) {
		// 壊れたファイルで起動を止めない。カーブ未設定として扱う
		return false;
	}

	FromJson(j);
	return true;
}
