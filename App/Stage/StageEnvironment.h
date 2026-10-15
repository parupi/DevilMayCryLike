#pragma once
#include <string>

/// <summary>
/// ステージの雰囲気（背景色・空・アンビエント・平行光・高さによる暗化）を、
/// エンジンの RenderEnvironment と SkySystem へ流し込む。
///
/// どの雰囲気にするかはステージデータの "environment" に名前で書く（例: "Cave"）。
/// 中身は GlobalVariables の Resource/GlobalVariables/Environment/Environment_<名前>.json にあり、
/// Debug では GlobalVariables のウィンドウからその場で調整できる（毎フレーム読み直すので即反映）。
/// 名前が無いステージ（トレーニングルームなど）は既定の見た目のまま。
/// </summary>
namespace StageEnvironment {

/// <summary>シーン開始時に呼ぶ。空の名前なら既定に戻すだけ</summary>
void Apply(const std::string& environmentName);

/// <summary>毎フレーム呼ぶ。調整中の値を反映する</summary>
void Update();

/// <summary>シーン終了時に呼ぶ。次のシーン（タイトル等）へ持ち越さない</summary>
void Clear();

/// <summary>松明の揺らぎの強さの倍率（雰囲気の "TorchFlickerStrength"。雰囲気が無ければ 1）。WallTorch が読む</summary>
float GetTorchFlickerScale();

} // namespace StageEnvironment
