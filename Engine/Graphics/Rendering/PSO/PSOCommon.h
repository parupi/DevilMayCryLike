#pragma once

enum class BlendMode {
	kNone,
	kNormal,
	kAdd,
	kSubtract,
	kMultiply,
	kScreen,
};

enum class OffScreenEffectType {
	kNone,
	kGray,
	kVignette,
	kSmooth,
	kGauss,
	kOutLine,
	kRadialBlur,          // ヒット位置を中心にした放射状ブラー
	kChromaticAberration, // 色収差
	kHitFlash,            // 画面全体の一瞬の発光
	kBloomBright,         // ブルーム：明るい部分の抽出
	kBloomBlur,           // ブルーム：分離ガウスブラー
	kBloomComposite,      // ブルーム：加算合成（加算ブレンドのPSO）
	kHeatDistortion,      // 熱の歪み（画面上の帯の範囲だけUVを揺らす）
	kColorGrading,        // 色の調整（暖色寄りなど）
	kSpeedLine,           // 集中線（突進などの速度感）
};
