#pragma once

// エンジン全サービスへの非所有ビュー
// GuchisFramework が所有・生成し、エンジン内部クラスへ注入する
// ゲームコードは引き続き GetInstance() を使用可能

class WindowManager;
class DirectXManager;
class PSOManager;
class Object3dManager;
class ModelManager;
class TextureManager;
class ParticleManager;
class RendererManager;
class LightManager;
class CameraManager;
class SkySystem;
class OffScreenManager;
class SceneManager;
class SpriteManager;
class TransitionManager;
class CollisionManager;
class PrimitiveLineDrawer;
class Audio;
class Input;
#ifdef _DEBUG
class ImGuiManager;
#endif

struct EngineContext {
    WindowManager*       winManager         = nullptr;
    DirectXManager*      dxManager          = nullptr;
    PSOManager*          psoManager         = nullptr;
    Object3dManager*     object3dManager    = nullptr;
    ModelManager*        modelManager       = nullptr;
    TextureManager*      textureManager     = nullptr;
    ParticleManager*     particleManager    = nullptr;
    RendererManager*     rendererManager    = nullptr;
    LightManager*        lightManager       = nullptr;
    CameraManager*       cameraManager      = nullptr;
    SkySystem*           skySystem          = nullptr;
    OffScreenManager*    offScreenManager   = nullptr;
    SceneManager*        sceneManager       = nullptr;
    SpriteManager*       spriteManager      = nullptr;
    TransitionManager*   transitionManager  = nullptr;
    CollisionManager*    collisionManager   = nullptr;
    PrimitiveLineDrawer* primitiveLineDrawer = nullptr;
    Audio*               audio              = nullptr;
    Input*               input              = nullptr;
#ifdef _DEBUG
    ImGuiManager*        imGuiManager       = nullptr;
#endif
};
