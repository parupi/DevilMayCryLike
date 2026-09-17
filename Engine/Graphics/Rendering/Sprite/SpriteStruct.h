#pragma once
#include <Graphics/Rendering/PSO/PSOManager.h>
#include <Math/Vector4.h>
#include <Math/Matrix4x4.h>

struct SpriteMaterial {
	Vector4 color; // offset  0 (16 bytes)
	Matrix4x4 uvTransform; // offset 16 (64 bytes)
	float dissolveThreshold; // offset 80 : -1.0 = disabled, 0.0-1.0 = dissolve amount
	float dissolveEdgeWidth; // offset 84 : edge glow width in noise space
	float radialFill; // offset 88 : -1.0 = disabled, 0.0-1.0 = 表示割合（上から時計回りに欠ける円形ゲージ）
	float padding; // offset 92 : HLSL float4 alignment
	Vector4 dissolveEdgeColor; // offset 96 : rgb = emissive color, a = intensity multiplier
};

struct SpriteRenderState {
	bool isVisible = true;
	BlendMode blendMode = BlendMode::kNormal;
};

/// スプライトの描画レイヤー。列挙の順に描画される（後ろのレイヤーほど手前に出る）。
///
/// 【シーンレイヤー】Background / Game
///   3Dシーンと一緒に描かれ、ポストエフェクトの影響を受ける。
///   ワールド上の何かに紐づく表示（ロックオンレティクルなど）はこちら。
/// 【UIレイヤー】UI / Persistent / Debug
///   画面に固定されるHUD。ポストエフェクトの外に出す対象。
///
/// SpriteManager に登録したスプライトは DrawSceneLayers() / DrawUILayers() が
/// 自動で描画する。各UIクラス側で個別に Draw() を呼ぶと二重描画になるので呼ばないこと。
enum class SpriteLayer {
	Background, // 背景として敷く全画面の絵など
	Game,       // ワールドに紐づく表示（ロックオンレティクル等）
	UI,         // 画面固定のHUD
	Persistent, // シーン切り替えで削除されない常駐スプライト用（フェード等）。UIより手前に出る
	Debug,
	Count,
};