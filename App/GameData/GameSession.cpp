#include "GameSession.h"
#include "EnemyCatalog.h"
#include "Stage/StageDocument.h"

#ifdef _DEBUG
#include <Debugger/GlobalVariables.h>
#endif

namespace {

GameMode g_mode = GameMode::Story;
std::string g_trainingEnemyClass;

#ifdef _DEBUG
// 起動設定の置き場。エディタの設定と同じディレクトリにまとめる
constexpr const char* kBootSettingsDirectory = "Editor";
constexpr const char* kBootSettingsGroup = "Training";
constexpr const char* kBootEnabledKey = "BootToTraining";
constexpr const char* kBootEnemyKey = "BootEnemyClass";

GameSession::BootSettings g_bootSettings;
bool g_bootSettingsLoaded = false;

void LoadBootSettings()
{
	if (g_bootSettingsLoaded) return;
	g_bootSettingsLoaded = true;

	GlobalVariables& gv = GlobalVariables::GetInstance();
	gv.CreateGroup(kBootSettingsGroup);
	gv.LoadFile(kBootSettingsDirectory, kBootSettingsGroup);

	if (gv.HasItem(kBootSettingsGroup, kBootEnabledKey)) {
		g_bootSettings.bootToTraining = gv.GetValueRef<bool>(kBootSettingsGroup, kBootEnabledKey);
	}
	if (gv.HasItem(kBootSettingsGroup, kBootEnemyKey)) {
		g_bootSettings.enemyClassName = gv.GetValueRef<std::string>(kBootSettingsGroup, kBootEnemyKey);
	}
}
#endif // _DEBUG

} // namespace

GameMode GameSession::GetMode()
{
	return g_mode;
}

void GameSession::BeginStory()
{
	g_mode = GameMode::Story;
	g_trainingEnemyClass.clear();
}

void GameSession::BeginTraining(const std::string& enemyClassName)
{
	g_mode = GameMode::Training;
	g_trainingEnemyClass = enemyClassName;
}

const std::string& GameSession::GetTrainingEnemyClass()
{
	return g_trainingEnemyClass;
}

std::string GameSession::GetStagePath()
{
	if (g_mode == GameMode::Training) {
		// トレーニングは StageDocument を経由しない。
		// Debug ビルドでは EditorStage.json が StageDocument の中身を上書きするので、
		// そちらを見るとエディタで開いていたステージへ飛んでしまう
		return kTrainingStagePath;
	}
	return StageDocument::GetPath();
}

std::string GameSession::ResolveBootScene()
{
#ifdef _DEBUG
	LoadBootSettings();

	if (g_bootSettings.bootToTraining) {
		// 保存されたクラス名が消えている（敵を作り直した等）場合は先頭にフォールバックする。
		// 見つからないまま入ると相手が居ないだけの部屋になって原因が分かりにくい
		const std::string& saved = g_bootSettings.enemyClassName;
		const bool valid = !saved.empty() && EnemyCatalog::Find(saved) != nullptr;
		BeginTraining(valid ? saved : EnemyCatalog::GetDefaultClassName());
		return "GAMEPLAY";
	}
#endif

	BeginStory();
	return "TITLE";
}

#ifdef _DEBUG

const GameSession::BootSettings& GameSession::GetBootSettings()
{
	LoadBootSettings();
	return g_bootSettings;
}

void GameSession::SetBootSettings(const BootSettings& settings)
{
	LoadBootSettings();
	g_bootSettings = settings;

	GlobalVariables& gv = GlobalVariables::GetInstance();
	gv.CreateGroup(kBootSettingsGroup);
	gv.SetValue(kBootSettingsGroup, kBootEnabledKey, g_bootSettings.bootToTraining);
	gv.SetValue(kBootSettingsGroup, kBootEnemyKey, g_bootSettings.enemyClassName);
	gv.SaveFile(kBootSettingsDirectory, kBootSettingsGroup);
}

#endif // _DEBUG
