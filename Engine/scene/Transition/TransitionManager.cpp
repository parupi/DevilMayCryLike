#include "TransitionManager.h"

TransitionManager* TransitionManager::instance = nullptr;
std::once_flag TransitionManager::initInstanceFlag;

TransitionManager* TransitionManager::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new TransitionManager();
		});
	return instance;
}

void TransitionManager::Finalize()
{
	transitions_.clear();
	current_ = nullptr;

	delete instance;
	instance = nullptr;
}

bool TransitionManager::AddTransition(std::unique_ptr<BaseTransition> transition)
{
	const std::string& name = transition->name;

	// すでに同じ名前のトランジションが登録されているか確認
	if (transitions_.find(name) != transitions_.end()) {
		// 既に存在している場合は追加しない
		return false;
	}

	// 新規登録
	transitions_[name] = std::move(transition);
	// 登録できたらTrue
	return true;
}

void TransitionManager::SetTransition(const std::string& transitionName)
{
	current_ = transitions_[transitionName].get();
}

BaseTransition* TransitionManager::GetTransition(const std::string& transitionName)
{
	return transitions_[transitionName].get();
}

void TransitionManager::DeleteAllTransition()
{
	transitions_.clear();
}

void TransitionManager::Play(bool isFadeOut)
{
	if (current_) {
		current_->Start(isFadeOut);
	}
}

void TransitionManager::Update()
{
	if (current_) {
		current_->Update();
	}
}

void TransitionManager::Draw()
{
	if (current_) {
		current_->Draw();
	}
}

bool TransitionManager::IsFinished() const
{
	return current_ ? current_->IsFinished() : true;
}
