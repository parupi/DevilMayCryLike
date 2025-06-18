#pragma once
#include "BaseRenderer.h"
#include <Model/Model.h>
#include <CameraManager.h>
#include <Camera/Camera.h>

class PrimitiveRenderer : public BaseRenderer {
public:
    enum class PrimitiveType {
        Plane,
        Ring,
        Cylinder
    };

    PrimitiveRenderer(const std::string& renderName, PrimitiveType type, std::string textureName);

    void Update(WorldTransform* parentTransform) override;
    void Draw(WorldTransform* parentTransform) override;

#ifdef _DEBUG
    void DebugGui(size_t index) override;
#endif
    WorldTransform* GetWorldTransform() const override { return localTransform_.get(); }

    BaseModel* GetModel() const override { return model_.get(); }
private:
    std::unique_ptr<Model> model_;
    std::unique_ptr<WorldTransform> localTransform_;
    Camera* camera_ = CameraManager::GetInstance()->GetActiveCamera();

    //void CreatePrimitiveModel(PrimitiveType type);
};