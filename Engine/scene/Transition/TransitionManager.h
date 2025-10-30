#pragma once
#include <string>
#include "BaseTransition.h"
#include <unordered_map>
#include <memory>
#include <mutex>
class TransitionManager
{
private:
	static TransitionManager* instance;
	static std::once_flag initInstanceFlag;

	TransitionManager() = default;
	~TransitionManager() = default;
	TransitionManager(TransitionManager&) = default;
	TransitionManager& operator=(TransitionManager&) = default;
public:
	// インスタンスの取得
	static TransitionManager* GetInstance();
	// 終了処理
	void Finalize();
	// 遷移の追加
	bool AddTransition(std::unique_ptr<BaseTransition> transition);
	// 使う遷移を名前から設定
	void SetTransition(const std::string& transitionName);
	// 遷移を取得
	BaseTransition* GetTransition(const std::string& transitionName);
	// 全てのシーン遷移の削除
	void DeleteAllTransition();
	// 再生
	void Play(bool isFadeOut);
	// 更新
	void Update();
	// 描画
	void Draw();
	// transitionが終わっているかどうか
	bool IsFinished() const;

	// 現在のトランジションを取得
	BaseTransition* GetCurrentTransition() const { return current_; }
private:
	std::unordered_map<std::string, std::unique_ptr<BaseTransition>> transitions_;
	BaseTransition* current_ = nullptr;
};

