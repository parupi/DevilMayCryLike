#include "VFXFile.h"
#include <filesystem>
#include <fstream>

namespace {

	const std::string kDirectory = "Resource/VFX/";
	const std::string kExtension = ".vfx.json";

} // namespace

namespace VFXFile {

	std::string MakeFilePath(const std::string& vfxName)
	{
		return kDirectory + vfxName + kExtension;
	}

	const char* ToString(PrimitiveType shape)
	{
		switch (shape) {
		case PrimitiveType::Ring:     return "Ring";
		case PrimitiveType::Cylinder: return "Cylinder";
		case PrimitiveType::Plane:
		default:                      return "Plane";
		}
	}

	PrimitiveType ShapeFromString(const std::string& name)
	{
		if (name == "Ring")     return PrimitiveType::Ring;
		if (name == "Cylinder") return PrimitiveType::Cylinder;
		return PrimitiveType::Plane; // 未知の名前は既定の Plane に倒す
	}

	bool Load(const std::string& vfxName, VFXDefinition& outDefinition)
	{
		std::ifstream file(MakeFilePath(vfxName));
		if (!file.is_open()) {
			return false;
		}

		nlohmann::json root;
		try {
			file >> root;
		} catch (const nlohmann::json::exception&) {
			// 壊れたファイルで起動を止めない。呼び出し側が false を見て従来経路へ落とす
			return false;
		}

		VFXDefinition definition;
		definition.name = root.value("Name", vfxName);

		// ── エミッター ──
		if (root.contains("Emitter") && root["Emitter"].is_object()) {
			const auto& emitterJson = root["Emitter"];
			definition.emitter.frequency = emitterJson.value("Frequency", 0.5f);
			definition.emitter.isActive = emitterJson.value("IsActive", false);
			definition.emitter.shapeModel = emitterJson.value("ShapeModel", std::string{});

			if (emitterJson.contains("Position") && emitterJson["Position"].is_array()
				&& emitterJson["Position"].size() >= 3) {
				definition.emitter.position = Vector3{
					emitterJson["Position"][0].get<float>(),
					emitterJson["Position"][1].get<float>(),
					emitterJson["Position"][2].get<float>()
				};
			}

			if (emitterJson.contains("Particles") && emitterJson["Particles"].is_array()) {
				for (const auto& element : emitterJson["Particles"]) {
					EmitterParticle particle;
					particle.name = element.value("ParticleName", std::string{});
					particle.count = element.value("Count", 1);
					particle.spawnRate = element.value("SpawnRate", 1.0f);
					if (!particle.name.empty()) {
						definition.emitter.particles.push_back(particle);
					}
				}
			}
		}

		// ── パーティクルグループ ──
		if (root.contains("Particles") && root["Particles"].is_object()) {
			for (auto it = root["Particles"].begin(); it != root["Particles"].end(); ++it) {
				VFXParticleDef particleDef;
				particleDef.name = it.key();

				const auto& value = it.value();
				particleDef.texture = value.value("Texture", std::string{ "white.png" });
				particleDef.shape = ShapeFromString(value.value("Shape", std::string{ "Plane" }));

				if (value.contains("Params") && value["Params"].is_object()) {
					particleDef.params = value["Params"];
				}
				if (value.contains("Curves")) {
					particleDef.curves.FromJson(value["Curves"]);
				}

				definition.particles.push_back(std::move(particleDef));
			}
		}

		outDefinition = std::move(definition);
		return true;
	}

	bool Save(const VFXDefinition& definition)
	{
		namespace fs = std::filesystem;

		const fs::path filePath(MakeFilePath(definition.name));
		const fs::path directory = filePath.parent_path();
		if (!directory.empty() && !fs::exists(directory)) {
			fs::create_directories(directory);
		}

		nlohmann::json root;
		root["Name"] = definition.name;

		nlohmann::json emitterJson;
		emitterJson["Frequency"] = definition.emitter.frequency;
		emitterJson["IsActive"] = definition.emitter.isActive;
		emitterJson["ShapeModel"] = definition.emitter.shapeModel;
		emitterJson["Position"] = nlohmann::json::array({
			definition.emitter.position.x,
			definition.emitter.position.y,
			definition.emitter.position.z });

		emitterJson["Particles"] = nlohmann::json::array();
		for (const EmitterParticle& particle : definition.emitter.particles) {
			emitterJson["Particles"].push_back({
				{ "ParticleName", particle.name },
				{ "Count", particle.count },
				{ "SpawnRate", particle.spawnRate } });
		}
		root["Emitter"] = std::move(emitterJson);

		root["Particles"] = nlohmann::json::object();
		for (const VFXParticleDef& particleDef : definition.particles) {
			nlohmann::json particleJson;
			particleJson["Texture"] = particleDef.texture;
			particleJson["Shape"] = ToString(particleDef.shape);
			particleJson["Params"] = particleDef.params;
			particleJson["Curves"] = particleDef.curves.ToJson();
			root["Particles"][particleDef.name] = std::move(particleJson);
		}

		std::ofstream file(filePath);
		if (!file.is_open()) {
			return false;
		}
		file << root.dump(4);
		return true;
	}

	std::vector<std::string> ListNames()
	{
		namespace fs = std::filesystem;

		std::vector<std::string> names;
		if (!fs::exists(kDirectory)) {
			return names;
		}

		for (const auto& entry : fs::directory_iterator(kDirectory)) {
			if (!entry.is_regular_file()) continue;

			// ".vfx.json" は二重拡張子なので stem() を2回取るのではなく末尾一致で判定する
			const std::string fileName = entry.path().filename().string();
			if (fileName.size() <= kExtension.size()) continue;
			if (fileName.compare(fileName.size() - kExtension.size(), kExtension.size(), kExtension) != 0) continue;

			names.push_back(fileName.substr(0, fileName.size() - kExtension.size()));
		}

		return names;
	}

} // namespace VFXFile
