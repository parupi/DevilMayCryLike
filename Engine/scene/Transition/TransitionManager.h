#pragma once
#include <string>
#include "BaseTransition.h"
#include <unordered_map>
#include <memory>
#include <mutex>
class TransitionManager
{
private:
	TransitionManager() = default;
	TransitionManager(const TransitionManager&) = delete;
	TransitionManager& operator=(const TransitionManager&) = delete;
public:
	// インスタンスの取得
	static TransitionManager& GetInstance();
	// 終了処理
	void Finalize();
	// 遷移の追加
	bool AddTransition(std::unique_ptr<BaseTransition> transition);
	// 使う遷移を名前から設定
	void SetTransition(const std::string& transitionName);
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

