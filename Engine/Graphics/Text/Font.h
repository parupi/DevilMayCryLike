#pragma once
#include "Math/Vector2.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct stbtt_fontinfo;

/// <summary>
/// 1文字ぶんの情報。位置と大きさはピクセル単位で、フォントを読み込んだときの
/// ピクセルサイズが基準になる（表示側で好きな大きさへ拡縮する）
/// </summary>
struct FontGlyph {
	Vector2 uvMin{};    ///< アトラス上の左上（0～1）
	Vector2 uvMax{};    ///< アトラス上の右下（0～1）
	Vector2 size{};     ///< 字形の大きさ。空白なら 0
	Vector2 bearing{};  ///< ペン位置から字形の左上へのずれ（yはベースラインから下向き）
	float advance = 0.0f; ///< 次の文字までペンを進める量
};

/// <summary>
/// TTF を実行時にラスタライズして、1枚のアトラスへ詰めていくフォント。
///
/// 使う文字が来たときに初めて焼くので、日本語のように文字種が多くても
/// 事前に「使う文字の一覧」を用意しなくてよい。
/// アトラスは CPU 側にも同じ絵を持っていて、描き足されたフレームだけ GPU へ上げ直す。
///
/// ラスタライズには stb_truetype を使う（実体は StbTrueType.cpp）
/// </summary>
class Font
{
public:
	Font();
	~Font();
	Font(const Font&) = delete;
	Font& operator=(const Font&) = delete;

	/// <summary>
	/// フォントを読み込む
	/// </summary>
	/// <param name="name">エンジン内で参照する名前。アトラスのテクスチャ名にも使う</param>
	/// <param name="ttfPath">リポジトリルートからの .ttf のパス</param>
	/// <param name="pixelHeight">焼くときの高さ(px)。表示サイズがこれより大きいとぼやける</param>
	/// <param name="atlasSize">アトラスの一辺(px)。RGBA なので size*size*4 バイトを CPU/GPU 双方に持つ</param>
	bool Initialize(const std::string& name, const std::string& ttfPath, float pixelHeight, uint32_t atlasSize = 1024);

	/// <summary>
	/// 字形を得る。まだ焼いていなければここで焼く。
	/// アトラスが満杯で焼けなかった場合だけ nullptr を返す
	/// </summary>
	const FontGlyph* GetGlyph(char32_t codePoint);

	/// <summary>隣り合う2文字のツメ（カーニング）</summary>
	float GetKerning(char32_t left, char32_t right) const;

	/// <summary>
	/// 描き足されたぶんを GPU へ上げ直す。何も増えていなければ何もしない。
	/// アトラスを読む描画より前に呼ぶこと
	/// </summary>
	void FlushAtlas();

	float GetPixelHeight() const { return pixelHeight_; }
	/// <summary>ベースラインから上端まで</summary>
	float GetAscent() const { return ascent_; }
	/// <summary>ベースラインから下端まで（下向きが正）</summary>
	float GetDescent() const { return descent_; }
	/// <summary>行送り</summary>
	float GetLineHeight() const { return lineHeight_; }
	/// <summary>TextureManager に登録してあるアトラスの名前</summary>
	const std::string& GetAtlasTextureName() const { return atlasTextureName_; }

private:
	// 字形を焼いてアトラスへ詰める
	const FontGlyph* Rasterize(char32_t codePoint);
	// アトラスの空きを棚（shelf）方式で取る。取れなければ false
	bool AllocateRect(uint32_t width, uint32_t height, uint32_t& outX, uint32_t& outY);

	std::unique_ptr<stbtt_fontinfo> info_;
	std::vector<uint8_t> ttfData_;   ///< stbtt_fontinfo が中を指し続けるので保持しておく
	std::unordered_map<char32_t, FontGlyph> glyphs_;

	std::vector<uint8_t> atlasPixels_; ///< RGBA。色は白固定で、字形の濃さをアルファに入れる
	uint32_t atlasSize_ = 0;
	// 棚の現在位置。横に並べていき、はみ出したら1段下げる
	uint32_t shelfX_ = 0;
	uint32_t shelfY_ = 0;
	uint32_t shelfHeight_ = 0;
	bool atlasDirty_ = false;
	bool atlasFull_ = false;

	float scale_ = 1.0f;
	float pixelHeight_ = 0.0f;
	float ascent_ = 0.0f;
	float descent_ = 0.0f;
	float lineHeight_ = 0.0f;

	std::string name_;
	std::string atlasTextureName_;

	/// <summary>字形どうしの隙間。線形補間で隣の字がにじむのを防ぐ</summary>
	static constexpr uint32_t kGlyphPadding = 2;
};
