#pragma once
#include "BaseRenderer.h"
#include "IDeferredDrawable.h"
#include <World3D/Object/Model/Model.h>
#include <World3D/Camera/CameraManager.h>
#include "World3D/Camera/BaseCamera.h"
#include "PrimitiveType.h"

class PrimitiveRenderer : public BaseRenderer, public IDeferredDrawable {
public:
    PrimitiveRenderer(const std::string& renderName, PrimitiveType type, std::string textureName);
    ~PrimitiveRenderer();

    void Update(WorldTransform* parentTransform) override;
    void Draw() override;
    void DrawGBuffer() override;
#ifdef _DEBUG
    void DebugGui(size_t index) override;
#endif
    WorldTransform* GetWorldTransform() const override { return localTransform_.get(); }
    BaseModel* GetModel() const override { return model_.get(); }

private:
    std::unique_ptr<Model> model_;
};