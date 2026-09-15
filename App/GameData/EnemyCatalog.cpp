#include "EnemyCatalog.h"

namespace {

// コライダーの大きさと配置スケールは、本編 Resource/Stage/Stage.json に置いている各クラスの値に合わせてある。
// ここを変えると本編と当たり判定・間合いが食い違うので、変えるならステージ側も一緒に直すこと
const std::vector<EnemyCatalogEntry> kEntries = {
	{
		"GruntMelee",
		"SKELETON",
		"剣を振る近接の雑魚。基本のコンボ確認向け",
		{ 0.75f, 0.75f, 0.75f },
		8.0f,
		1.0f,
	},
	{
		"BossKnight",
		"DRAGON",
		"噛みつき・叩きつけ・突進で戦うボス。スーパーアーマーあり",
		{ 0.8524f, 1.031f, 0.9279f },
		16.0f, // 本編と同じ2倍の体なので、叩きつけの円（半径4.8m）より十分外から始める
		2.0f,
	},
	{
		"TutorialDummy",
		"DUMMY",
		"攻撃してこない練習台。動きだけ相手にしたいとき用",
		{ 1.0f, 1.0f, 1.0f },
		8.0f,
		1.0f,
	},
};

const std::string kEmpty;

} // namespace

const std::vector<EnemyCatalogEntry>& EnemyCatalog::GetEntries()
{
	return kEntries;
}

const EnemyCatalogEntry* EnemyCatalog::Find(const std::string& className)
{
	for (const auto& entry : kEntries) {
		if (entry.className == className) {
			return &entry;
		}
	}
	return nullptr;
}

const std::string& EnemyCatalog::GetDefaultClassName()
{
	if (kEntries.empty()) {
		return kEmpty;
	}
	return kEntries.front().className;
}
