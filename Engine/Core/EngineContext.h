#pragma once

// エンジン全サービスへの非所有ビュー
// GuchisFramework が所有・生成し、エンジン内部クラスへ注入する
// ゲームコードは引き続き GetInstance() を使用可能

class DirectXManager;
class PSOManager;
class Object3dManager;
class LightManager;
class CameraManager;
class SkySystem;
class OffScreenManager;
class SceneManager;
class SpriteManager;
class TransitionManager;
class CollisionManager;
class PrimitiveLineDrawer;
#ifdef _DEBUG
class ImGuiManager;
#endif

struct EngineContext {
    DirectXManager*      dxManager         = nullptr;
    PSOManager*          psoManager         = nullptr;
    Object3dManager*     object3dManager    = nullptr;
    LightManager*        lightManager       = nullptr;
    CameraManager*       cameraManager      = nullptr;
    SkySystem*           skySystem          = nullptr;
    OffScreenManager*    offScreenManager   = nullptr;
    SceneManager*        sceneManager       = nullptr;
    SpriteManager*       spriteManager      = nullptr;
    TransitionManager*   transitionManager  = nullptr;
    CollisionManager*    collisionManager   = nullptr;
    PrimitiveLineDrawer* primitiveLineDrawer = nullptr;
#ifdef _DEBUG
    ImGuiManager*        imGuiManager       = nullptr;
#endif
};
