#pragma once
#include "ActionGuideTypes.h"

#include <Math/Vector4.h>
#include <string>

struct ActionGuideSettings;

/// <summary>アイコンの形</summary>
enum class InputIconShape : uint8_t {
	Circle,  ///< 丸（A / B / X / Y）
	Wide,    ///< 横長の角丸（RB / RT、キーボードのキー）
};

/// <summary>1つのアイコンの見た目</summary>
struct InputIcon {
	InputIconShape shape = InputIconShape::Circle;
	Vector4 fillColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	Vector4 textColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	std::string text;
	/// <summary>横幅の倍率（高さに対して）。文字数の多いキー（SPACE など）ほど広い</summary>
	float widthScale = 1.0f;
};

/// <summary>
/// 論理入力から、今の入力デバイスに合うアイコンと組み合わせの文字を決める（仕様書 §5.6）。
/// UI 側にボタンの文字（X / Y / RT など）を書かないための窓口。
///
/// パッドとキーボードの判断は PlayerInput と同じく「パッドが繋がっているか」で行う
/// （攻撃・移動はパッドを繋ぐとキーボードが効かなくなるため、表記もそれに合わせる）
/// </summary>
class InputIconProvider
{
public:
	/// <summary>今パッドの表記で出すか</summary>
	static bool UsePad();

	/// <summary>入力のアイコン</summary>
	static InputIcon Resolve(InputAction action, bool usePad);

	/// <summary>アイコンの左に出す組み合わせ（"RB + ↑ +" など）。無ければ空</summary>
	static std::string PrefixText(const InputBinding& binding, bool usePad);

	/// <summary>押し方の言葉（"長押し" など）。普通に押すだけなら空</summary>
	static std::string GestureText(const InputBinding& binding, const ActionGuideSettings& settings);
};
