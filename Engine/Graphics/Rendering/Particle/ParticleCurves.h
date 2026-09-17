#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "Curve.h"
#include "Gradient.h"

/// <summary>
/// パーティクルグループ1つぶんの時間変化カーブ。
///
/// GlobalVariables は int / float / bool / Vector3 しか保存できずカーブを表現できないため、
/// これらだけは別ファイル（Resource/Particle/&lt;GroupName&gt;.curve.json）に保存する。
///
/// **すべて空の場合は「カーブ未設定」**として扱い、パーティクルは従来どおり
/// FadeType（None / Alpha / ScaleShrink）の挙動で動く。
/// 既存のパーティクルグループはカーブファイルを持たないので、この経路で後方互換が保たれる。
/// </summary>
struct ParticleCurves
{
	/// <summary>スケール倍率。生成時スケールに掛かる（1.0 = 変化なし）</summary>
	Curve sizeCurve;
	/// <summary>アルファ倍率。生成時アルファに掛かる</summary>
	Curve alphaCurve;
	/// <summary>色。生成時カラーに成分ごとに掛かる</summary>
	Gradient colorGradient;

	/// <summary>カーブが1つも設定されていないか（＝従来挙動で動かす）</summary>
	bool IsEmpty() const {
		return sizeCurve.IsEmpty() && alphaCurve.IsEmpty() && colorGradient.IsEmpty();
	}

	/// <summary>Resource/Particle/&lt;groupName&gt;.curve.json へ保存する</summary>
	void Save(const std::string& groupName) const;
	/// <summary>
	/// Resource/Particle/&lt;groupName&gt;.curve.json から読み込む。
	/// ファイルが無ければ何もしない（空のまま＝従来挙動）。
	/// </summary>
	/// <returns>読み込めたら true</returns>
	bool Load(const std::string& groupName);

	/// <summary>保存先のファイルパスを組み立てる</summary>
	static std::string MakeFilePath(const std::string& groupName);

	/// <summary>
	/// { "SizeCurve": [...], "AlphaCurve": [...], "ColorGradient": [...] } の形にする。
	/// 単体の .curve.json でも .vfx.json への埋め込みでも同じ形を使う。
	/// </summary>
	nlohmann::json ToJson() const;
	/// <summary>ToJson() の形から読む。無いキーは触らない</summary>
	void FromJson(const nlohmann::json& object);
};
