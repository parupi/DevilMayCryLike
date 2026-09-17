#include "BaseEvent.h"
#include "EventManager.h"

BaseEvent::BaseEvent(std::string objectName, EventType type) : Object3d(objectName) {
	type_ = type;
	// 生成経路（ステージ読み込み / エディタ）によらず必ず登録されるよう、ここで自己登録する。
	// name_ は Object3d のコンストラクタで入っているので参照して問題ない
	EventManager::GetInstance().AddEvent(this);
}

BaseEvent::~BaseEvent() {
	// エディタから削除されたときに EventManager がぶら下がりを掴まないように外す
	EventManager::GetInstance().RemoveEvent(this);
}
