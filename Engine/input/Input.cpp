#include "Input.h"
#include <stdexcept>
#include <string>

Input* Input::instance = nullptr;
std::once_flag Input::initInstanceFlag;

Input* Input::GetInstance()
{
    std::call_once(initInstanceFlag, []() {
        instance = new Input();
        });
    return instance;
}

// 初期化処理
void Input::Initialize() {
    HRESULT hr;

    // DirectInputの初期化
    hr = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&dInput_, nullptr);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to initialize DirectInput");
    }

    // キーボードデバイスの作成
    hr = dInput_->CreateDevice(GUID_SysKeyboard, &devKeyboard_, nullptr);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create keyboard device");
    }

    // マウスデバイスの作成
    hr = dInput_->CreateDevice(GUID_SysMouse, &devMouse_, nullptr);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create mouse device");
    }

    // キーボードの初期化
    devKeyboard_->SetDataFormat(&c_dfDIKeyboard);
    devKeyboard_->SetCooperativeLevel(hwnd_, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    devKeyboard_->Acquire();

    // マウスの初期化
    devMouse_->SetDataFormat(&c_dfDIMouse2);
    devMouse_->SetCooperativeLevel(hwnd_, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    devMouse_->Acquire();

    // ジョイスティックの初期化
    SetupJoysticks();
}

void Input::Finalize()
{
    if (devKeyboard_) {
        devKeyboard_->Unacquire();
    }
    if (devMouse_) {
        devMouse_->Unacquire();
    }
    delete instance;
    instance = nullptr;
}

// ジョイスティックのセットアップ
void Input::SetupJoysticks() {
    dInput_->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, this, DIEDFL_ATTACHEDONLY);
}

// 毎フレームの更新処理
void Input::Update() {
    // 前回のキーとマウスの状態を保存
    keyPre_ = key_;
    mousePre_ = mouse_;

    // キーボード状態の更新
    devKeyboard_->GetDeviceState(sizeof(key_), key_.data());

    // マウス状態の更新
    devMouse_->GetDeviceState(sizeof(DIMOUSESTATE2), &mouse_);

    // ジョイスティック状態の更新
    for (auto& joystick : devJoysticks_) {
        joystick.statePre_ = joystick.state_;

        if (joystick.type_ == PadType::XInput) {
            XInputGetState(0, &joystick.state_.xInput_);
        }
        else {
            joystick.device_->Poll();
            joystick.device_->GetDeviceState(sizeof(DIJOYSTATE2), &joystick.state_.directInput_);
        }
    }

    //Vector2 stick = GetXInputLeftStick();
    //bool a = IsXInputAPressed();
    //OutputDebugStringA(("LeftStick: " + std::to_string(stick.x) + ", " + std::to_string(stick.y) + "\n").c_str());
    //if (a) OutputDebugStringA("Aボタン押下中\n");
}

// キーが押されているかのチェック
bool Input::PushKey(BYTE keyNumber) const {
    return key_[keyNumber] & 0x80;
}

// キーがトリガーかのチェック
bool Input::TriggerKey(BYTE keyNumber) const {
    return (key_[keyNumber] & 0x80) && !(keyPre_[keyNumber] & 0x80);
}

// 全マウス情報の取得
const DIMOUSESTATE2& Input::GetAllMouse() const {
    return mouse_;
}

// マウスボタンが押されているかのチェック
bool Input::IsPressMouse(int32_t buttonNumber) const {
    return mouse_.rgbButtons[buttonNumber] & 0x80;
}

// マウスボタンがトリガーかのチェック
bool Input::IsTriggerMouse(int32_t buttonNumber) const {
    return (mouse_.rgbButtons[buttonNumber] & 0x80) && !(mousePre_.rgbButtons[buttonNumber] & 0x80);
}

// マウス移動量の取得
Input::MouseMove Input::GetMouseMove() {
    MouseMove move = { mouse_.lX, mouse_.lY, mouse_.lZ };
    return move;
}

// ホイールスクロール量の取得
int32_t Input::GetWheel() const {
    return mouse_.lZ;
}

// マウス位置の取得
const Vector2& Input::GetMousePosition() const {
    return mousePosition_;
}

// ジョイスティック状態の取得 (DirectInput)
bool Input::GetJoystickState(int32_t stickNo, DIJOYSTATE2& out) const {
    if (stickNo >= devJoysticks_.size()) return false;
    out = devJoysticks_[stickNo].state_.directInput_;
    return true;
}

// 前回のジョイスティック状態の取得 (DirectInput)
bool Input::GetJoystickStatePrevious(int32_t stickNo, DIJOYSTATE2& out) const {
    if (stickNo >= devJoysticks_.size()) return false;
    out = devJoysticks_[stickNo].statePre_.directInput_;
    return true;
}

// ジョイスティック状態の取得 (XInput)
bool Input::GetJoystickState(int32_t stickNo, XINPUT_STATE& out) const {
    if (stickNo >= devJoysticks_.size()) return false;
    out = devJoysticks_[stickNo].state_.xInput_;
    return true;
}

// 前回のジョイスティック状態の取得 (XInput)
bool Input::GetJoystickStatePrevious(int32_t stickNo, XINPUT_STATE& out) const {
    if (stickNo >= devJoysticks_.size()) return false;
    out = devJoysticks_[stickNo].statePre_.xInput_;
    return true;
}

// ジョイスティックのデッドゾーン設定
void Input::SetJoystickDeadZone(int32_t stickNo, int32_t deadZoneL, int32_t deadZoneR) {
    if (stickNo >= devJoysticks_.size()) return;
    devJoysticks_[stickNo].deadZoneL_ = deadZoneL;
    devJoysticks_[stickNo].deadZoneR_ = deadZoneR;
}

// 接続されているジョイスティックの数を取得
size_t Input::GetNumberOfJoysticks() {
    return devJoysticks_.size();
}
bool Input::IsXInputAPressed(int32_t stickNo) const {
    XINPUT_STATE state{};
    if (XInputGetState(stickNo, &state) == ERROR_SUCCESS) {
        return (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
    }
    return false;
}

Vector2 Input::GetXInputLeftStick(int32_t stickNo) const {
    XINPUT_STATE state{};
    if (XInputGetState(stickNo, &state) != ERROR_SUCCESS) {
        return { 0.0f, 0.0f };
    }

    // 正規化 [-1.0, 1.0]
    float normLX = static_cast<float>(state.Gamepad.sThumbLX) / 32767.0f;
    float normLY = static_cast<float>(state.Gamepad.sThumbLY) / 32767.0f;

    // デッドゾーン処理（必要に応じて調整可能）
    const float deadZone = 0.2f;
    if (std::abs(normLX) < deadZone) normLX = 0.0f;
    if (std::abs(normLY) < deadZone) normLY = 0.0f;

    return { normLX, normLY };
}


// ジョイスティック列挙のコールバック
BOOL CALLBACK Input::EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext) noexcept {
    auto input = static_cast<Input*>(pContext);
    Microsoft::WRL::ComPtr<IDirectInputDevice8> joystick;

    // ジョイスティックデバイスを作成
    if (FAILED(input->dInput_->CreateDevice(pdidInstance->guidInstance, &joystick, nullptr))) {
        return DIENUM_CONTINUE;
    }

    // デバイスをリストに追加
    Joystick newJoystick = {};
    newJoystick.device_ = joystick;
    newJoystick.type_ = PadType::DirectInput;
    input->devJoysticks_.push_back(newJoystick);

    return DIENUM_CONTINUE;
}
