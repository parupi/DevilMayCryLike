#pragma once
#include "3d/Object/Model/ModelStructs.h"
class SkySystem
{
public:
	void Initialize();

	void Draw();

private:
	void CreateSkyBoxVertex();

	std::vector<VertexData> vertexData_;
};

