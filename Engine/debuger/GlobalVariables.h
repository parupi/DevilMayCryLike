#pragma once
#include <variant>
#include <map>
#include <string>
#include <fstream>
#include <filesystem>
#include <cassert>
#include "math/Vector3.h"
#include "math/Vector4.h"

#ifdef _DEBUG
#include <imgui/imgui.h>
#include <Windows.h>
#endif
#include <nlohmann/json.hpp>

class GlobalVariables {
public:
	using json = nlohmann::json;

	// 値の型
	using Value = std::variant<int32_t, float, Vector3, Vector4, bool>;

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
	static GlobalVariables* GetInstance();

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
	void SaveFile(const std::string& groupName);
	void LoadFile(const std::string& groupName);
	void LoadFiles();

private:
	GlobalVariables() = default;
	~GlobalVariables() = default;
	GlobalVariables(const GlobalVariables&) = delete;
	GlobalVariables& operator=(const GlobalVariables&) = delete;

	std::map<std::string, Group> datas_;
	const std::string kDirectoryPath = "resource/GlobalVariables/";
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
	group.items[key] = Item{ Value(static_cast<NormalizedT>(value)) };
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
