#include "PrimitiveFactory.h"
#include "MeshGenerator.h"
#include "Graphics/Resource/TextureManager.h"

// PrimitiveFactory.cpp
std::unique_ptr<Model> PrimitiveFactory::Create(PrimitiveType type, std::string textureName) {
    MeshData meshData;
    MaterialData mat;

    // メッシュ生成
    switch (type) {
    case PrimitiveType::Plane:
        meshData = MeshGenerator::CreatePlane();
        mat.name = "PlaneMaterial";
        break;
    case PrimitiveType::Ring:
        meshData = MeshGenerator::CreateRing();
        mat.name = "RingMaterial";
        break;
    case PrimitiveType::Cylinder:
        meshData = MeshGenerator::CreateCylinder();
        mat.name = "CylinderMaterial";
        break;
    }

    // マテリアルの個別設定
    mat.Ns = 20.0f;  // Shininess（鏡面反射の強さ）
    mat.Ka = { 0.1f, 0.1f, 0.1f };  // 環境光（薄く黒っぽい）
    mat.Kd = { 1.0f, 1.0f, 1.0f };  // 拡散反射（ベースカラー）
    mat.Ks = { 0.2f, 0.2f, 0.2f };  // 鏡面反射（白に近いハイライト）
    mat.Ni = 1.0f;     // 屈折率（未使用なら1.0fでOK）
    mat.d = 1.0f;      // 不透明度（完全に不透明）
    mat.illum = 2;     // ライティングモデル（Phongなど。とりあえず2）

    // テクスチャ
    mat.textureFilePath = textureName;
    TextureManager::GetInstance()->LoadTexture(textureName);
    mat.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureName);

    meshData.materialIndex = 0;

    auto model = std::make_unique<Model>();
    model->InitializeFromMesh(meshData, mat);
    return model;
}