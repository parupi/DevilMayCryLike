#include "GlobalVariables.h"
#include <fstream>
#include "Windows.h"
#ifdef _DEBUG
#include <imgui/imgui.h>
#endif 

using namespace std;

GlobalVariables& GlobalVariables::GetInstance() {
	static GlobalVariables instance;

	return instance;
}

void GlobalVariables::CreateGroup(const string& groupName) {
	// 指定名のオブジェクトがなければ追加する
	datas_[groupName];
}

void GlobalVariables::RemoveItem(const std::string& groupName, const std::string& key) {
	auto itGroup = datas_.find(groupName);
	if (itGroup == datas_.end()) return;
	itGroup->second.items.erase(key);
}

GlobalVariables::json GlobalVariables::ExportGroup(const std::string& groupName) const {
	json object = json::object();

	auto itGroup = datas_.find(groupName);
	if (itGroup == datas_.end()) {
		return object;
	}

	for (const auto& [itemName, item] : itGroup->second.items) {
		if (holds_alternative<int32_t>(item.value)) {
			object[itemName] = get<int32_t>(item.value);
		} else if (holds_alternative<float>(item.value)) {
			object[itemName] = get<float>(item.value);
		} else if (holds_alternative<Vector3>(item.value)) {
			const Vector3 value = get<Vector3>(item.value);
			object[itemName] = json::array({ value.x, value.y, value.z });
		} else if (holds_alternative<Vector4>(item.value)) {
			const Vector4 value = get<Vector4>(item.value);
			object[itemName] = json::array({ value.x, value.y, value.z, value.w });
		} else if (holds_alternative<bool>(item.value)) {
			object[itemName] = get<bool>(item.value);
		} else if (holds_alternative<std::string>(item.value)) {
			object[itemName] = get<std::string>(item.value);
		}
	}

	return object;
}

void GlobalVariables::ImportGroup(const std::string& groupName, const json& object) {
	if (!object.is_object()) {
		return;
	}

	for (auto itItem = object.begin(); itItem != object.end(); ++itItem) {
		const string& itemName = itItem.key();

		// bool は is_number_integer() が false なので、int より先に判定しなくてよい
		if (itItem->is_number_integer()) {
			SetValue(groupName, itemName, itItem->get<int32_t>());
		} else if (itItem->is_number_float()) {
			SetValue(groupName, itemName, static_cast<float>(itItem->get<double>()));
		} else if (itItem->is_array() && itItem->size() == 3) {
			SetValue(groupName, itemName, Vector3{ itItem->at(0), itItem->at(1), itItem->at(2) });
		} else if (itItem->is_array() && itItem->size() == 4) {
			SetValue(groupName, itemName, Vector4{ itItem->at(0), itItem->at(1), itItem->at(2), itItem->at(3) });
		} else if (itItem->is_boolean()) {
			SetValue(groupName, itemName, itItem->get<bool>());
		} else if (itItem->is_string()) {
			SetValue(groupName, itemName, itItem->get<std::string>());
		}
	}
}

void GlobalVariables::SaveFile(const std::string& directoryName, const string& groupName) {
	// 未登録チェック
	assert(datas_.find(groupName) != datas_.end());

	// 保存先を覚えておく（SaveAllFiles / ReloadAllFiles が使う）
	groupDirectories_[groupName] = directoryName;

	json root = json::object();
	root[groupName] = ExportGroup(groupName);

	// ディレクトリが無ければ作成する
	filesystem::path dir = std::filesystem::path(kDirectoryPath) / directoryName;
	if (!filesystem::exists(dir)) {
		filesystem::create_directory(dir);
	}
	// 書き込むJSONファイルのフルパスを合成する
	string filePath = kDirectoryPath + directoryName + "/" + groupName + ".json";
	// 書き込み用ファイナルストリーム
	ofstream ofs;
	// ファイルを書き込み用に開く
	ofs.open(filePath);
	// ファイルオープン失敗？
	if (ofs.fail()) {
		string message = "Failed open data file for write";
		MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
		assert(0);
		return;
	}
	// ファイルにjson文字列を書き込む（インデックス幅4）
	ofs << setw(4) << root << endl;
	// ファイルを閉じる
	ofs.close();
}

size_t GlobalVariables::SaveAllFiles() {
	size_t saved = 0;
	for (const auto& [groupName, directoryName] : groupDirectories_) {
		// グループごと消えている可能性があるので、SaveFile のassertに当たらないよう確認する
		if (datas_.find(groupName) == datas_.end()) {
			continue;
		}
		SaveFile(directoryName, groupName);
		++saved;
	}
	return saved;
}

size_t GlobalVariables::ReloadAllFiles() {
	// LoadFile が groupDirectories_ を書き換えるので、走査用にコピーを取る
	const std::map<std::string, std::string> targets = groupDirectories_;

	size_t loaded = 0;
	for (const auto& [groupName, directoryName] : targets) {
		LoadFile(directoryName, groupName);
		++loaded;
	}
	return loaded;
}

void GlobalVariables::LoadFiles(const std::string& directoryName) {
	std::filesystem::path dir = std::filesystem::path(kDirectoryPath) / directoryName;

	if (!std::filesystem::exists(dir)) {
		return;
	}

	for (const auto& entry : std::filesystem::directory_iterator(dir)) {
		if (entry.path().extension() != ".json") {
			continue;
		}

		std::string groupName = entry.path().stem().string();
		LoadFile(directoryName, groupName);
	}
}

bool GlobalVariables::HasItem(const std::string& groupName, const std::string& key) const {
	auto groupIt = datas_.find(groupName);
	if (groupIt == datas_.end()) {
		return false;
	}

	return groupIt->second.items.contains(key);
}

std::vector<std::string> GlobalVariables::GetGroupNames(const std::string& directoryName) const {
	std::vector<std::string> result;

	std::filesystem::path dir = std::filesystem::path(kDirectoryPath) / directoryName;

	if (!std::filesystem::exists(dir)) {
		return result;
	}

	for (const auto& entry : std::filesystem::directory_iterator(dir)) {
		if (!entry.is_regular_file()) {
			continue;
		}

		const auto& path = entry.path();
		if (path.extension() != ".json") {
			continue;
		}

		// 拡張子を除いたファイル名
		result.push_back(path.stem().string());
	}

	return result;
}


void GlobalVariables::LoadFile(const std::string& directoryName, const std::string& groupName) {
	// 読み込み元＝保存先として覚えておく（ファイルが無くても、次に保存する場所はここ）
	groupDirectories_[groupName] = directoryName;

	// 読み込むJSONファイルのフルパスを合成する
	string filePath = kDirectoryPath + directoryName + "/" + groupName + ".json";
	// 読み込む用ファイルストリーム
	ifstream ifs;
	// ファイルを読み込むように開く
	ifs.open(filePath);
	// ファイルオープン失敗
	if (ifs.fail()) {
		return;
	}

	json root;

	// json文字列からjsonのデータ構造に展開
	ifs >> root;
	// ファイルを閉じる
	ifs.close();

	// グループ名を検索
	json::iterator itGroup = root.find(groupName);

	// 未登録チェック
	assert(itGroup != root.end());

	ImportGroup(groupName, *itGroup);
}
