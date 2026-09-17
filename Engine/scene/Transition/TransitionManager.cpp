#include "TransitionManager.h"


TransitionManager& TransitionManager::GetInstance() {
	static TransitionManager instance;
	return instance;
}

void TransitionManager::Finalize() {
	transitions_.clear();
	current_ = nullptr;

}

bool TransitionManager::AddTransition(std::unique_ptr<BaseTransition> transition) {
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

void TransitionManager::SetTransition(const std::string& transitionName) {
	// operator[] だと未登録の名前で空の要素を作ってしまい、
	// 以降 AddTransition が「登録済み」と誤判定するので find で引く
	auto it = transitions_.find(transitionName);
	current_ = (it != transitions_.end()) ? it->second.get() : nullptr;
}

bool TransitionManager::HasTransition(const std::string& transitionName) const {
	return transitions_.find(transitionName) != transitions_.end();
}

BaseTransition* TransitionManager::GetTransition(const std::string& transitionName) {
	auto it = transitions_.find(transitionName);
	return (it != transitions_.end()) ? it->second.get() : nullptr;
}

void TransitionManager::DeleteAllTransition() {
	transitions_.clear();
}

void TransitionManager::Play(bool isFadeOut) {
	if (current_) {
		current_->Start(isFadeOut);
	}
}

void TransitionManager::Update() {
	if (current_) {
		current_->Update();
	}
}

void TransitionManager::Draw() {
	if (current_) {
		current_->Draw();
	}
}

bool TransitionManager::IsFinished() const {
	return current_ ? current_->IsFinished() : true;
}