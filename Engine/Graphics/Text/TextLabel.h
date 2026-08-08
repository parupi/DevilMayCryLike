#pragma once
#include "Font.h"
#include "Graphics/Rendering/Sprite/SpriteStruct.h"
#include "Math/Matrix4x4.h"
#include "Math/Vector2.h"
#include "Math/Vector4.h"

#include <d3d12.h>
#include <cstdint>
#include <string>

class SpriteManager;

/// <summary>横の揃え方。基準になるのは SetPosition で渡した位置</summary>
enum class TextAlignX {
	Left,
	Center,
	Right,
};

/// <summary>縦の揃え方。Middle は文字列全体の高さの中心が基準位置に来る</summary>
enum class TextAlignY {
	Top,
	Middle,
	Bottom,
};

/// <summary>
/// 文字列を描くもの。スプライトと同じレイヤーに並び、SpriteManager が自動で描画する。
///
/// 文字ぶんの四角形を1本の頂点バッファへまとめて詰めるので、
/// 何文字あってもドローコールは1回（影を付けた場合は2回）で済む。
/// PSO とルートシグネチャはスプライトのものをそのまま使っている。
///
/// 生成は SpriteManager::CreateTextLabel() から行うこと
/// </summary>
class TextLabel
{
public:
	TextLabel(const std::string& name, Font* font, SpriteLayer layer);
	~TextLabel();
	TextLabel(const TextLabel&) = delete;
	TextLabel& operator=(const TextLabel&) = delete;

	friend class SpriteManager;

	/// <summary>頂点と行列を作り直す。毎フレーム呼ぶ（中身が変わっていなければ軽い）</summary>
	void Update();
	/// <summary>描画。SpriteManager が呼ぶので、UI側から呼ぶと二重描画になる</summary>
	void Draw();

	/// <summary>表示する文字列（UTF-8）。ソースに直接書いた日本語がそのまま使える</summary>
	void SetText(const std::string& text);
	const std::string& GetText() const { return text_; }

	void SetPosition(const Vector2& position) { position_ = position; }
	const Vector2& GetPosition() const { return position_; }

	/// <summary>表示する文字の高さ(px)。フォントを焼いた高さより大きくするとぼやける</summary>
	void SetFontSize(float pixelSize);
	float GetFontSize() const { return fontSize_; }

	void SetColor(const Vector4& color) { color_ = color; }
	const Vector4& GetColor() const { return color_; }

	void SetAlign(TextAlignX alignX, TextAlignY alignY);

	/// <summary>行送りの倍率。1.0 でフォント本来の行間</summary>
	void SetLineSpacing(float scale);

	/// <summary>
	/// 影（同じ文字列をずらして後ろに描く）。明るい背景でも文字が沈まないようにするためのもの
	/// </summary>
	void SetShadow(bool enabled, const Vector2& offset = { 2.0f, 2.0f }, const Vector4& color = { 0.0f, 0.0f, 0.0f, 0.7f });

	/// <summary>組んだ結果の大きさ(px)。中央寄せや枠の計算に使う</summary>
	const Vector2& GetSize() const { return measuredSize_; }

	SpriteRenderState& GetRenderState() { return renderState_; }
	const SpriteRenderState& GetRenderState() const { return renderState_; }
	SpriteLayer GetLayer() const { return layer_; }
	const std::string& GetName() const { return name_; }

private:
	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
	};

	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
	};

	// 文字列から四角形を組み立てて頂点バッファへ書く
	void BuildVertices();
	// 文字数ぶんのバッファを用意する。足りなければ作り直す
	void EnsureCapacity(uint32_t glyphCount);
	// 定数バッファを作る
	void CreateConstantBuffers();
	// 1行の幅を測る
	float MeasureLineWidth(const std::u32string& line) const;

	std::string name_;
	Font* font_ = nullptr;
	SpriteLayer layer_ = SpriteLayer::UI;
	SpriteRenderState renderState_{};

	std::string text_;
	std::u32string codePoints_;

	Vector2 position_{};
	Vector2 measuredSize_{};
	Vector4 color_{ 1.0f, 1.0f, 1.0f, 1.0f };
	float fontSize_ = 32.0f;
	float lineSpacing_ = 1.0f;
	TextAlignX alignX_ = TextAlignX::Left;
	TextAlignY alignY_ = TextAlignY::Top;

	bool hasShadow_ = false;
	Vector2 shadowOffset_{ 2.0f, 2.0f };
	Vector4 shadowColor_{ 0.0f, 0.0f, 0.0f, 0.7f };

	/// <summary>文字列や大きさが変わったら立てる。立っている間だけ頂点を組み直す</summary>
	bool isDirty_ = true;

	// ==========================
	// GPUリソース
	// ==========================
	uint32_t vertexHandle_ = 0;
	uint32_t indexHandle_ = 0;
	uint32_t materialHandle_ = 0;
	uint32_t transformHandle_ = 0;
	uint32_t shadowMaterialHandle_ = 0;
	uint32_t shadowTransformHandle_ = 0;
	bool hasBuffers_ = false;

	VertexData* vertexData_ = nullptr;
	uint32_t* indexData_ = nullptr;
	SpriteMaterial* materialData_ = nullptr;
	SpriteMaterial* shadowMaterialData_ = nullptr;
	TransformationMatrix* transformData_ = nullptr;
	TransformationMatrix* shadowTransformData_ = nullptr;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

	/// <summary>今のバッファが何文字ぶんか</summary>
	uint32_t glyphCapacity_ = 0;
	/// <summary>実際に描く文字数（空白などを除いた四角形の数）</summary>
	uint32_t drawGlyphCount_ = 0;
};
