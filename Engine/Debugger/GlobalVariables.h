#pragma once
#include <variant>
#include <map>
#include <string>
#include <fstream>
#include <filesystem>
#include <cassert>
#include "Math/Vector3.h"
#include "Math/Vector4.h"

#ifdef _DEBUG
#include <imgui/imgui.h>
#include <Windows.h>
#endif
#include <nlohmann/json.hpp>

class GlobalVariables {
public:
	using json = nlohmann::json;

	// 値の型
	using Value = std::variant<int32_t, float, Vector3, Vector4, bool, std::string>;

	// 項目構造体
	struct Item {
		Value value;
	};

	// グループ構造体
	struct Group {
		std::map<std::string, Item> items;
	};

	// 型正規化（int → int32_t）
	template<typename T> struct NormalizeType { using Type = T; };
	template<> struct NormalizeType<int> { using Type = int32_t; };

	// インスタンス取得
	static GlobalVariables& GetInstance();

	// グループ作成
	void CreateGroup(const std::string& groupName);

	// 項目追加（テンプレート）
	template<typename T>
	void AddItem(const std::string& groupName, const std::string& key, const T& value);

	// 値のセット（テンプレート）
	template<typename T>
	void SetValue(const std::string& groupName, const std::string& key, const T& value);

	// 値の取得参照（テンプレート）
	template <typename T>
	T& GetValueRef(const std::string& groupName, const std::string& key);

	// アイテムの切り離し
	void RemoveItem(const std::string& groupName, const std::string& key);

	// ファイル入出力
	void SaveFile(const std::string& directoryName, const std::string& groupName);
	void LoadFile(const std::string& directoryName, const std::string& groupName);
	void LoadFiles(const std::string& directoryName);

	/// <summary>
	/// 一度でも SaveFile / LoadFile したことのあるグループを、すべて元のディレクトリへ書き出す。
	/// グループごとの保存先はコード側の慣習でバラバラなので、
	/// 「どこに保存するか」は入出力したときの実績から覚えている。
	/// </summary>
	/// <returns>書き出したグループ数</returns>
	size_t SaveAllFiles();

	/// <summary>
	/// SaveAllFiles と同じ対象を、ファイルから読み直す。
	/// 編集した内容は破棄されるので注意。
	/// </summary>
	/// <returns>読み直したグループ数</returns>
	size_t ReloadAllFiles();

	/// <summary>グループ名 → 保存先ディレクトリ名。エディタが一覧を出すのに使う</summary>
	const std::map<std::string, std::string>& GetGroupDirectories() const { return groupDirectories_; }

	/// <summary>
	/// グループの全項目を { キー: 値 } の json オブジェクトにする。
	/// 未登録のグループなら空オブジェクトを返す。
	/// 個々の項目名を知らなくても丸ごと持ち運べるので、
	/// .vfx.json のように別形式のファイルへ埋め込みたいときに使う。
	/// </summary>
	json ExportGroup(const std::string& groupName) const;

	/// <summary>
	/// json オブジェクトの各項目をグループへ流し込む（既存の値は上書きされる）。
	/// **オブジェクトに無いキーは触らない**ので、後から増えたパラメータは
	/// AddItem の既定値のまま残る（古いファイルを読んでも壊れない）。
	/// </summary>
	void ImportGroup(const std::string& groupName, const json& object);

	// アイテムの存在確認
	bool HasItem(const std::string& groupName, const std::string& key) const;

	// ディレクトリ内のグループ名一覧取得
	std::vector<std::string> GetGroupNames(const std::string& directoryName) const;
private:
	GlobalVariables() = default;
	~GlobalVariables() = default;
	GlobalVariables(const GlobalVariables&) = delete;
	GlobalVariables& operator=(const GlobalVariables&) = delete;

	std::map<std::string, Group> datas_;
	// グループごとの保存先ディレクトリ。SaveFile / LoadFile が呼ばれるたびに記録する
	std::map<std::string, std::string> groupDirectories_;
	const std::string kDirectoryPath = "Resource/GlobalVariables/";
};

// テンプレート関数実装

template<typename T>
void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, const T& value) {
	using NormalizedT = typename NormalizeType<T>::Type;
	Group& group = datas_[groupName];
	if (group.items.find(key) == group.items.end()) {
		SetValue<NormalizedT>(groupName, key, value);
	}
}

template<typename T>
void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, const T& value) {
	using NormalizedT = typename NormalizeType<T>::Type;
	Group& group = datas_[groupName];
	group.items[key] = Item{ Value(NormalizedT(value)) };
}

template <typename T>
T& GlobalVariables::GetValueRef(const std::string& groupName, const std::string& key) {
	using NormalizedT = typename NormalizeType<T>::Type;

	assert(datas_.find(groupName) != datas_.end());
	Group& group = datas_.at(groupName);
	assert(group.items.find(key) != group.items.end());
	Item& item = group.items.at(key);
	assert(std::holds_alternative<NormalizedT>(item.value));
	return std::get<NormalizedT>(item.value);
}
