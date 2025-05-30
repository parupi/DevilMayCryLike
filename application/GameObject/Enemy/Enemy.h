#pragma once
#include "3d/Object/Object3d.h"
class Enemy : public Object3d
{
public:
	Enemy();
	~Enemy() override = default;
	// 初期
	void Initialize() override;
	// 更新
	void Update() override;
	// 描画
	void Draw() override;
	void DrawEffect();

#ifdef _DEBUG
	void DebugGui() override;
#endif // _DEBUG

};

