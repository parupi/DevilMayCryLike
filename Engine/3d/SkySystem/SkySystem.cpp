#include "SkySystem.h"

void SkySystem::Initialize()
{
    CreateSkyBoxVertex();
}

void SkySystem::Draw()
{

}

void SkySystem::CreateSkyBoxVertex()
{
    // 頂点数を必要分設定（6面 × 2三角形 × 3頂点 = 36）
    vertexData_.resize(36);

    // 右面 (+X)
    vertexData_[0].position = { 1.0f, 1.0f, 1.0f, 1.0f };
    vertexData_[1].position = { 1.0f, 1.0f, -1.0f, 1.0f };
    vertexData_[2].position = { 1.0f, -1.0f, 1.0f, 1.0f };
    vertexData_[3].position = { 1.0f, -1.0f, 1.0f, 1.0f };
    vertexData_[4].position = { 1.0f, 1.0f, -1.0f, 1.0f };
    vertexData_[5].position = { 1.0f, -1.0f, -1.0f, 1.0f };

    // 左面 (-X)
    vertexData_[6].position = { -1.0f, 1.0f, -1.0f, 1.0f };
    vertexData_[7].position = { -1.0f, 1.0f, 1.0f, 1.0f };
    vertexData_[8].position = { -1.0f, -1.0f, -1.0f, 1.0f };
    vertexData_[9].position = { -1.0f, -1.0f, -1.0f, 1.0f };
    vertexData_[10].position = { -1.0f, 1.0f, 1.0f, 1.0f };
    vertexData_[11].position = { -1.0f, -1.0f, 1.0f, 1.0f };

    // 前面 (+Z)
    vertexData_[12].position = { -1.0f, 1.0f, 1.0f, 1.0f };
    vertexData_[13].position = { 1.0f, 1.0f, 1.0f, 1.0f };
    vertexData_[14].position = { -1.0f, -1.0f, 1.0f, 1.0f };
    vertexData_[15].position = { -1.0f, -1.0f, 1.0f, 1.0f };
    vertexData_[16].position = { 1.0f, 1.0f, 1.0f, 1.0f };
    vertexData_[17].position = { 1.0f, -1.0f, 1.0f, 1.0f };

    // 背面 (-Z)
    vertexData_[18].position = { 1.0f, 1.0f, -1.0f, 1.0f };
    vertexData_[19].position = { -1.0f, 1.0f, -1.0f, 1.0f };
    vertexData_[20].position = { 1.0f, -1.0f, -1.0f, 1.0f };
    vertexData_[21].position = { 1.0f, -1.0f, -1.0f, 1.0f };
    vertexData_[22].position = { -1.0f, 1.0f, -1.0f, 1.0f };
    vertexData_[23].position = { -1.0f, -1.0f, -1.0f, 1.0f };

    // 上面 (+Y)
    vertexData_[24].position = { -1.0f, 1.0f, -1.0f, 1.0f };
    vertexData_[25].position = { 1.0f, 1.0f, -1.0f, 1.0f };
    vertexData_[26].position = { -1.0f, 1.0f, 1.0f, 1.0f };
    vertexData_[27].position = { -1.0f, 1.0f, 1.0f, 1.0f };
    vertexData_[28].position = { 1.0f, 1.0f, -1.0f, 1.0f };
    vertexData_[29].position = { 1.0f, 1.0f, 1.0f, 1.0f };

    // 下面 (-Y)
    vertexData_[30].position = { -1.0f, -1.0f, 1.0f, 1.0f };
    vertexData_[31].position = { 1.0f, -1.0f, 1.0f, 1.0f };
    vertexData_[32].position = { -1.0f, -1.0f, -1.0f, 1.0f };
    vertexData_[33].position = { -1.0f, -1.0f, -1.0f, 1.0f };
    vertexData_[34].position = { 1.0f, -1.0f, 1.0f, 1.0f };
    vertexData_[35].position = { 1.0f, -1.0f, -1.0f, 1.0f };
}