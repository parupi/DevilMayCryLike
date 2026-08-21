#pragma once
#include <mutex>
#include <vector>
#include "BaseCollider.h"
#include "AABBCollider.h"
#include "SphereCollider.h"
#include "OBBCollider.h"
#include <set>
class CollisionManager
{
private:
	CollisionManager() = default;
	CollisionManager(const CollisionManager&) = delete;
	CollisionManager& operator=(const CollisionManager&) = delete;

public:
	static CollisionManager& GetInstance();

	void Initialize();
	// 終了処理
	void Finalize();
	void Update();

	void DeleteAllCollider();

	void Draw();

	void AddCollider(std::unique_ptr<BaseCollider> collider);

	BaseCollider* FindCollider(std::string colliderName);

	void RemoveDeadObjects();

	const std::vector<BaseCollider*>& GetCurrentHits(BaseCollider* collider) const;

	// 押し出しに使う軸の絞り込み
	enum class PenetrationAxis {
		Any,            // 15軸すべてから最小の重なりを選ぶ（地形との押し出し）
		HorizontalOnly, // 水平な軸だけを候補にする（キャラ同士の分離）
	};

	// mover を blocker から押し出すためのMTV(最小移動ベクトル)を計算する
	// AABB/OBBの組み合わせに対応（Sphereは非対応）
	//
	// axisMode に HorizontalOnly を渡すと、重なっているかの判定は全軸で行ったうえで
	// 「水平な軸の中で最小の重なり」を返す。キャラ同士は上下の重なりが最小になりやすく、
	// そのまま押すと相手に踏まれた側が床下へ押し込まれるため
	PenetrationResult CalculatePenetration(BaseCollider* mover, BaseCollider* blocker,
		PenetrationAxis axisMode = PenetrationAxis::Any);

	// 全コライダーの一覧を取得する（地面スナップなどの空間クエリ用）
	const std::vector<std::unique_ptr<BaseCollider>>& GetColliders() const { return colliders_; }

private:
	using ColliderPair = std::pair<BaseCollider*, BaseCollider*>;

	// コライダーの順序を固定する比較関数（ペアが常に同じ順になるように）
	struct ColliderPairCompare {
		bool operator()(const ColliderPair& a, const ColliderPair& b) const {
			return std::tie(a.first, a.second) < std::tie(b.first, b.second);
		}
	};

	std::set<ColliderPair, ColliderPairCompare> currentCollisions_;
	std::set<ColliderPair, ColliderPairCompare> previousCollisions_;
	
	// ブロードフェーズ用のワールドAABB
	struct ColliderBounds {
		Vector3 min{};
		Vector3 max{};
	};
	// コライダーを包むワールドAABBを求める（形状ごとの詳細判定の前に使う）
	static void CalcWorldBounds(const BaseCollider* collider, ColliderBounds& out);
	static bool OverlapBounds(const ColliderBounds& a, const ColliderBounds& b);

	void CheckAllCollisions();
	bool CheckCollision(BaseCollider* a, BaseCollider* b);

	bool CheckAABBToAABBCollision(AABBCollider* a, AABBCollider* b);
	bool CheckSphereToSphereCollision(SphereCollider* a, SphereCollider* b);
	bool CheckOBBToOBBCollision(OBBCollider* a, OBBCollider* b);
	bool CheckOBBToAABBCollision(OBBCollider* obb, AABBCollider* aabb);

	// 分離軸定理(SAT)による、有向ボックス同士のMTV計算の共通実装
	// axesA/axesBはそれぞれ3要素の正規直交基底
	PenetrationResult CalculateBoxPenetration(
		const Vector3& centerA, const Vector3 axesA[3], const Vector3& halfA,
		const Vector3& centerB, const Vector3 axesB[3], const Vector3& halfB,
		PenetrationAxis axisMode);

private:
	std::vector<std::unique_ptr<BaseCollider>> colliders_;
	// colliders_ と同じ並びのワールドAABB。毎フレーム作り直す（確保を使い回すためメンバに持つ）
	std::vector<ColliderBounds> broadPhaseBounds_;

};

