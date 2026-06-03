#pragma once
#include <World3D/WorldTransform.h>
#include <World3D/Object/Object3d.h>
#include <memory>
class DebugSphere
{
public:
	// 初期化
	void Initialize();
	// 終了
	void Finalize();
	// 更新
	void Update(const Vector3& position);
	// 描画
	void Draw();

private:
	WorldTransform transform_;
	std::unique_ptr<Object3d> object_;
};

