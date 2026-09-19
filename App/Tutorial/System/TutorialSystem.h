#pragma once
#include <unordered_map>
#include <memory>
#include "../Tutorial.h"
#include "../Service/TutorialService.h"
#include "Tutorial/Decoration/TutorialDecoration.h"

class TutorialSystem : public TutorialService {
public:
	TutorialSystem() = default;
	~TutorialSystem() = default;
	/// <summary>
	/// 初期化。
	/// </summary>
	/// <param name="enabled">
	/// チュートリアルを流すか。false を渡すと **表示物を1つも作らない**（最初から完了扱いになる）。
	/// アルファ0で隠す方式にしていた頃は、OPTION で切っていても画面左に出てしまうことがあり、
	/// 使わない説明文も6項目ぶん作ることになっていた。作らなければどちらも起きない
	/// </param>
	void Initialize(bool enabled);
	// 更新
	void Update();
	// チュートリアルの開始
	void StartTutorial(TutorialState state) override;
	// チュートリアルの終了
	void EndTutorial();
	// 進行度を進める
	void StepTutorial(TutorialState state) override;
	// 全チュートリアルが完了したか（表示が消えきってから true になる）
	bool IsAllFinished() const override { return isAllFinished_; }
	// チュートリアルを流さずに完了扱いにする
	void SkipAllTutorials() override { isFinishing_ = false; isAllFinished_ = true; }
	// チュートリアルを流すシーンか
	bool IsEnabled() const override { return enabled_; }
private:
	// 現在のチュートリアルを終了し、次のチュートリアルへ自動的に進める
	void AdvanceTutorial();

	// チュートリアルを流すシーンか。false のときは tutorials_ も decoration_ も空のまま
	bool enabled_ = true;

	TutorialState state_ = TutorialState::Move; // 現在のチュートリアルの状態

	// チュートリアルのマップ
	std::unordered_map<TutorialState, std::unique_ptr<Tutorial>> tutorials_;
	// 現在のチュートリアル
	Tutorial* currentTutorial_ = nullptr;
	// 全チュートリアルを完了したか（最後の種類まで進み切り、表示も消えきったら true）
	bool isAllFinished_ = false;
	// 最後の種類まで進み切って、表示のフェードアウト待ちか（Update で完了へ移す）。
	// この間は IsAllFinished() が false のままなので、TutorialDummy はまだ倒せない
	bool isFinishing_ = false;
	// 装飾表示用のクラス
	std::unique_ptr<TutorialDecoration> decoration_ = nullptr;
};

