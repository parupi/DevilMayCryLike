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

	// mover を blocker から押し出すためのMTV(最小移動ベクトル)を計算する
	// AABB/OBBの組み合わせに対応（Sphereは非対応）
	PenetrationResult CalculatePenetration(BaseCollider* mover, BaseCollider* blocker);

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
		const Vector3& centerB, const Vector3 axesB[3], const Vector3& halfB);

private:
	std::vector<std::unique_ptr<BaseCollider>> colliders_;

};

