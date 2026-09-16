#pragma once
#include <string>
#include <vector>
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Particle.h"
#include "ParticleCurves.h"
#include "World3D/Object/Renderer/PrimitiveType.h"

// 発生位置オフセットに対する放射方向の速度モード
enum class RadialMode {
	None = 0,     // 通常（min/maxVelocity のランダム速度）
	Converge = 1, // 収束: 寿命が尽きる瞬間に発生中心へ到達する速度を与える
	Diverge = 2,  // 拡散: 発生中心から外向きに RadialSpeed で飛ばす
};

// ビルボードの種類。
// 「ビルボードするかどうか」は従来どおり isBillboard が決め、これは「どう向くか」だけを決める。
// 既定値(0)が従来の Screen なので、この項目を持たない既存グループの見た目は変わらない。
enum class BillboardType {
	Screen = 0,   // 常にカメラの方を向く（従来の挙動）
	AxisY = 1,    // Y軸周りだけ回る。炎・光の柱など「立っている」ものに使う
	Velocity = 2, // 進行方向に軸を合わせて引き伸ばす。火花の尾を引く表現
};

struct ParticleParameters {
	Vector2 translateX;
	Vector2 translateY;
	Vector2 translateZ;
	Vector2 rotateX;
	Vector2 rotateY;
	Vector2 rotateZ;
	Vector2 scaleX;
	Vector2 scaleY;
	Vector2 scaleZ;
	Vector2 velocityX;
	Vector2 velocityY;
	Vector2 velocityZ;
	Vector2 lifeTime;
	Vector3 colorMin;
	Vector3 colorMax;
	bool isBillboard;

	// ── 回転の速さ[rad/s]（min/max）──
	// 生成時にこの範囲から1つ選び、寿命のあいだ rotate に足し続ける。
	// ビルボード(Screen/AxisY)は Z だけを「画面内の傾き」として使う（Velocity は進行方向が意味を持つので回さない）。
	// Orient To Direction は Z を「向けた軸まわりの回転」として使う
	Vector2 angularVelocityX;
	Vector2 angularVelocityY;
	Vector2 angularVelocityZ;
	int radialMode = 0;          // RadialMode
	float radialSpeed = 1.0f;    // Converge: 速度倍率 / Diverge: 外向き速度(m/s)

	// ── 簡易物理 ──
	Vector3 gravity{ 0.0f, 0.0f, 0.0f }; // 毎秒加算される加速度[m/s^2]
	float drag = 1.0f;                   // 1秒あたりの速度維持率(1.0=減衰なし, 0.5=毎秒半減)

	// ── 方向付き発生 ──
	// Emit に方向が渡され、かつ useDirectional が真のときだけ有効。
	// その場合 minVelocity/maxVelocity と RadialMode は使われず、
	// 「方向を軸とした半角 spreadDegrees のコーン内 × speedMin〜speedMax」で速度が決まる。
	bool useDirectional = false;
	float speedMin = 0.0f;
	float speedMax = 0.0f;
	float spreadDegrees = 0.0f;     // コーンの半角[度]。0=まっすぐ / 180=全方向
	bool orientToDirection = false; // メッシュ自体を方向に向ける（Billboard時は無効）

	// ── ビルボードの種類 ──
	// isBillboard が false のときはどれも使われない。
	// これらは「生成時に確定する値」ではなく描画時に毎フレーム読む値なので、
	// エディタで切り替えると発生済みのパーティクルにも即座に反映される。
	int billboardType = 0;        // BillboardType
	float velocityStretch = 0.0f; // Velocity時、速度1m/sあたり何倍に引き伸ばすか(0=伸ばさない)

	// ── テクスチャアニメーション（スプライトシート）──
	// columns * rows が 1 以下なら何もしない＝テクスチャ全体をそのまま使う（既存グループ）
	int animColumns = 1;
	int animRows = 1;
	float animFps = 0.0f;   // 0以下 = 寿命いっぱいでシートを1周させる
	bool animLoop = true;   // 最後のコマまで行ったら先頭へ戻る。falseなら最後のコマで止まる

	// ── ノイズ（炎・煙の質感）──
	// ノイズテクスチャ（既定は FireNoise.jpg）を時間でスクロールさせて、形・色・消え方を揺らす。
	// noiseEnabled が false なら何もしない＝既存グループの見た目は変わらない。
	// 色はノイズから、形（アルファ）は本来のテクスチャから取るので、
	// smoke.png のように「RGBが黒でアルファだけ」の素材にもそのまま色が乗る
	bool noiseEnabled = false;
	Vector3 noiseTiling{ 1.0f, 1.0f, 0.0f };  // xy だけ使う。ノイズを何回繰り返すか
	Vector3 noiseScroll{ 0.0f, -1.0f, 0.0f }; // xy だけ使う。UV/秒
	float noiseDistortion = 0.0f;   // 本来のテクスチャのUVをノイズで揺らす量（0.05〜0.2くらい）
	float noiseColorBlend = 1.0f;   // 0=本来のテクスチャの色 / 1=ノイズの色
	bool noiseGrayscale = false;    // ノイズの色ではなく明るさだけを使う（煙向け）
	float noiseErosion = 0.0f;      // 寿命の終わりへ向けて暗い所から削れていく量（0=削らない / 1.2くらいで最後に消える）
	float noiseErosionSoftness = 0.15f; // 削れる境目のぼかし幅
	float emissiveIntensity = 1.0f; // 色に掛ける明るさ。重なった炎が白く飛ぶように1より上げる

	// ── 描画順 ──
	// グループは SortOrder の小さい順に描く（同じなら名前順）。
	// 煙（通常ブレンド）を先に、炎（加算）を後に描くと、炎が煙に埋もれない
	int sortOrder = 0;
	// グループ内の粒をカメラから遠い順に並べて描く。通常ブレンドの煙の前後崩れを防ぐ（加算には不要）
	bool sortByDepth = false;
	// ソフトパーティクル: 地面や壁との距離がこれ[m]より近い部分を薄くする。0=使わない
	float softDistance = 0.0f;
};

struct ParticleGroup
{
	std::vector<Particle> particles;
	ParticleParameters params;
	// 生成時に指定されたテクスチャ名。.vfx.json へ書き出すために覚えておく
	// （描画に使うインデックスは ParticleRenderState 側が持っている）
	std::string texturePath;
	// ノイズテクスチャ名。空なら ParticleManager の既定（FireNoise.jpg）を使う
	std::string noiseTexturePath;
	// 時間変化カーブ。params と違い GlobalVariables ではなく別ファイル管理なので、
	// 毎フレームの LoadParticleParameters() で上書きされないようここに置いている。
	ParticleCurves curves;
	PrimitiveType shape = PrimitiveType::Plane;
};