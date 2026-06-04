#include "GameData.h"

GameData& GameData::GetInstance()
{
	static GameData instance;
	return instance;
}
