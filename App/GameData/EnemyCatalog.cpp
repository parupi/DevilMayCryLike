#include "EnemyCatalog.h"

namespace {

// コライダーの大きさは Resource/Stage/Test.json に置かれている各クラスの値に合わせてある。
// ここを変えると本編との当たり判定が食い違うので、変えるならステージ側も一緒に直すこと
const std::vector<EnemyCatalogEntry> kEntries = {
	{
		"GruntMelee",
		"SKELETON",
		"剣を振る近接の雑魚。基本のコンボ確認向け",
		{ 1.0f, 1.0f, 1.0f },
		8.0f,
	},
	{
		"BossKnight",
		"DRAGON",
		"噛みつき・叩きつけ・突進で戦うボス。スーパーアーマーあり",
		{ 1.3f, 1.3f, 1.3f },
		12.0f,
	},
	{
		"TutorialDummy",
		"DUMMY",
		"攻撃してこない練習台。動きだけ相手にしたいとき用",
		{ 1.0f, 1.0f, 1.0f },
		8.0f,
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
