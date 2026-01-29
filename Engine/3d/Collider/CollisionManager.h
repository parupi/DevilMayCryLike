#pragma once
#include <mutex>
#include <vector>
#include "BaseCollider.h"
#include "AABBCollider.h"
#include "SphereCollider.h"
#include <set>
class CollisionManager
{
private:
	static CollisionManager* instance;
	static std::once_flag initInstanceFlag;

	CollisionManager() = default;
	~CollisionManager() = default;
	CollisionManager(CollisionManager&) = default;
	CollisionManager& operator=(CollisionManager&) = default;

public:
	static CollisionManager* GetInstance();

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

private:
	std::vector<std::unique_ptr<BaseCollider>> colliders_;

};

