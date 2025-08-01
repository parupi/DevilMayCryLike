#include "GlobalVariables.h"
#include <fstream>
#include "Windows.h"
#ifdef _DEBUG
#include <imgui/imgui.h>
#endif 


using namespace std;

GlobalVariables* GlobalVariables::GetInstance() {
	static GlobalVariables instance;

	return &instance;
}

void GlobalVariables::CreateGroup(const string& groupName) {
	// 指定名のオブジェクトがなければ追加する
	datas_[groupName];
}

void GlobalVariables::RemoveItem(const std::string& groupName, const std::string& key)
{
	auto itGroup = datas_.find(groupName);
	if (itGroup == datas_.end()) return;
	itGroup->second.items.erase(key);
}

void GlobalVariables::SaveFile(const string& groupName) {
	// グループを検索
	map<string, Group>::iterator itGroup = datas_.find(groupName);

	// 未登録チェック
	assert(itGroup != datas_.end());

	json root;
	root = json::object();

	// jsonオブジェクト登録
	root[groupName] = json::object();

	// 各項目について
	for (map<string, Item>::iterator itItem = itGroup->second.items.begin();
		itItem != itGroup->second.items.end(); ++itItem) {

		// 項目名を取得
		const string& itemName = itItem->first;
		// 項目の参照を取得
		Item& item = itItem->second;

		// int32_t型の値を保持していれば
		if (holds_alternative<int32_t>(item.value)) {
			// int32_t型の値を登録
			root[groupName][itemName] = get<int32_t>(item.value);
		}
		// float型の値を保持していれば
		else if (holds_alternative<float>(item.value)) {
			// float型の値を登録
			root[groupName][itemName] = get<float>(item.value);
		}
		// Vector3型の値を保持していれば
		else if (holds_alternative<Vector3>(item.value)) {
			// float型のjson配列を登録
			Vector3 value = get<Vector3>(item.value);
			root[groupName][itemName] = json::array({ value.x, value.y, value.z });
		}// bool型の値を保持していれば
		else if (std::holds_alternative<bool>(item.value)) {
			// bool型の値を登録
			root[groupName][itemName] = std::get<bool>(item.value);
		}
	}
	// ディレクトリが無ければ作成する
	filesystem::path dir(kDirectoryPath);
	if (!filesystem::exists(kDirectoryPath)) {
		filesystem::create_directory(kDirectoryPath);
	}
	// 書き込むJSONファイルのフルパスを合成する
	string filePath = kDirectoryPath + groupName + ".json";
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

void GlobalVariables::LoadFiles() {
	// ローカル変数の保存先ファイルパス
	const string directoryPath = kDirectoryPath;

	// ディレクトリがなければスキップする
	if (!std::filesystem::exists(directoryPath)) {
		return;
	}

	filesystem::directory_iterator dir_it(directoryPath);
	for (const filesystem::directory_entry& entry : dir_it) {
		// ファイルパスを取得
		const filesystem::path& filePath = entry.path();

		// ファイル拡張子を取得
		string extension = filePath.extension().string();
		// .jsonファイル以外はスキップ
		if (extension.compare(".json") != 0) {
			continue;
		}
		// ファイル読み込み
		LoadFile(filePath.stem().string());
	}
}


void GlobalVariables::LoadFile(const std::string& groupName) {
	// 読み込むJSONファイルのフルパスを合成する
	string filePath = kDirectoryPath + groupName + ".json";
	// 読み込む用ファイルストリーム
	ifstream ifs;
	// ファイルを読み込むように開く
	ifs.open(filePath);
	// ファイルオープン失敗
	if (ifs.fail()) {
		string message = "Failed open data file for write";
		MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
		assert(0);
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

	// 各アイテムについて
	for (json::iterator itItem = itGroup->begin(); itItem != itGroup->end(); ++itItem) {
		// アイテム名そ取得
		const string& itemName = itItem.key();
		// int32_t型の値を保持していれば
		if (itItem->is_number_integer()) {
			// int型の値を登録
			int32_t value = itItem->get<int32_t>();
			SetValue(groupName, itemName, value);
		} // float型の値を保持していれば
		else if (itItem->is_number_float()) {
			// int型の値を登録
			double value = itItem->get<double>();
			SetValue(groupName, itemName, static_cast<float>(value));
		} // Vector3型の値を保持していれば
		else if (itItem->is_array() && itItem->size() == 3) {
			// float型のjson配列登録
			Vector3 value = { itItem->at(0), itItem->at(1), itItem->at(2) };
			SetValue(groupName, itemName, value);
		} // bool型の値を保持していれば
		else if (itItem->is_boolean()) {
			// bool型の値を登録
			bool value = itItem->get<bool>();
			SetValue(groupName, itemName, value);
		}
	}
}
