#include "GameData.h"

GameData* GameData::instance = nullptr;
std::once_flag GameData::initInstanceFlag;

GameData* GameData::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new GameData();
		});
	return instance;
}
