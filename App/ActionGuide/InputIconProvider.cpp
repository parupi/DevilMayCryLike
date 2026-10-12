#include "InputIconProvider.h"
#include "ActionGuideSettings.h"

#include <Input/Input.h>

namespace {
	// Xbox 系パッドのボタンの色（少し彩度を落として、UI の中で浮かないようにしている）
	constexpr Vector4 kColorA = { 0.38f, 0.74f, 0.27f, 1.0f };
	constexpr Vector4 kColorX = { 0.27f, 0.50f, 0.90f, 1.0f };
	constexpr Vector4 kColorY = { 0.93f, 0.76f, 0.20f, 1.0f };
	constexpr Vector4 kColorShoulder = { 0.22f, 0.22f, 0.24f, 1.0f };
	// 明るい床の上でも埋もれないよう、キーは真っ白にせず少し落とす
	constexpr Vector4 kColorKey = { 0.78f, 0.78f, 0.76f, 1.0f };
	constexpr Vector4 kTextDark = { 0.08f, 0.08f, 0.08f, 1.0f };
	constexpr Vector4 kTextLight = { 0.95f, 0.95f, 0.95f, 1.0f };

	InputIcon PadButton(const char* text, const Vector4& color) {
		InputIcon icon;
		icon.shape = InputIconShape::Circle;
		icon.fillColor = color;
		icon.textColor = kTextDark;
		icon.text = text;
		return icon;
	}
	InputIcon Shoulder(const char* text) {
		InputIcon icon;
		icon.shape = InputIconShape::Wide;
		icon.fillColor = kColorShoulder;
		icon.textColor = kTextLight;
		icon.text = text;
		icon.widthScale = 1.45f;
		return icon;
	}
	InputIcon Key(const char* text, float widthScale = 1.0f) {
		InputIcon icon;
		icon.shape = InputIconShape::Wide;
		icon.fillColor = kColorKey;
		icon.textColor = kTextDark;
		icon.text = text;
		icon.widthScale = widthScale;
		return icon;
	}
}

bool InputIconProvider::UsePad()
{
	return Input::GetInstance().IsConnected();
}

InputIcon InputIconProvider::Resolve(InputAction action, bool usePad)
{
	// 割り当ては PlayerInput / LockOnInput と同じ。変えたらここも直すこと
	if (usePad) {
		switch (action) {
		case InputAction::Attack:       return PadButton("Y", kColorY);
		case InputAction::StrongAttack: return PadButton("X", kColorX);
		case InputAction::Jump:         return PadButton("A", kColorA);
		case InputAction::Dodge:        return Shoulder("RT");
		case InputAction::LockOn:       return Shoulder("RB");
		default:                        return Key("?");
		}
	}
	switch (action) {
	case InputAction::Attack:       return Key("J");
	case InputAction::StrongAttack: return Key("K");
	case InputAction::Jump:         return Key("SPACE", 2.3f);
	case InputAction::Dodge:        return Key("SHIFT", 2.1f);
	case InputAction::LockOn:       return Key("P");
	default:                        return Key("?");
	}
}

std::string InputIconProvider::PrefixText(const InputBinding& binding, bool usePad)
{
	std::string text;
	if (binding.withLockOn) {
		text += usePad ? "RB + " : "P + ";
	}
	// スティックの前後。キーボードは移動キーの W / S（PlayerInput がスティックの代わりに使う）
	switch (binding.stick) {
	case StickHint::Forward: text += usePad ? "↑ + " : "W + "; break;
	case StickHint::Back:    text += usePad ? "↓ + " : "S + "; break;
	default: break;
	}
	return text;
}

std::string InputIconProvider::GestureText(const InputBinding& binding, const ActionGuideSettings& settings)
{
	switch (binding.gesture) {
	case InputGesture::Hold:      return settings.textHold;
	case InputGesture::Release:   return settings.textRelease;
	case InputGesture::WaitPress: return settings.textWait;
	default:                      return std::string();
	}
}
