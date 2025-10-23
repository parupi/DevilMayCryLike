#pragma once
#include <string>
#include "BaseEvent.h"

/// <summary>
/// イベントを生成するファクトリークラス  
/// クラス名から対応するイベントを生成する
/// </summary>
class EventFactory
{
public:
	/// <summary>
	/// クラス名に対応したイベントオブジェクトを生成
	/// </summary>
	/// <param name="className">生成したいクラス名</param>
	/// <param name="objectName">オブジェクト名</param>
	/// <returns>生成されたイベントのユニークポインタ</returns>
	static std::unique_ptr<BaseEvent> Create(const std::string& className, const std::string& objectName);
};
