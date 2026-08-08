#pragma once
#include "Font.h"

#include <memory>
#include <string>
#include <unordered_map>

/// <summary>
/// フォントを名前で持っておく置き場。
///
/// TextureManager と同じく、初期化のどこかで一度読み込んでおけば
/// あとは名前で引ける。既定フォントを決めておくと TextLabel 側で毎回指定せずに済む
/// </summary>
class FontManager
{
public:
	static FontManager& GetInstance();

	/// <summary>
	/// フォントを読み込む。同じ名前が既にあればそれを返す
	/// </summary>
	/// <param name="pixelHeight">焼くときの高さ(px)。表示サイズの最大値くらいにしておくと綺麗に出る</param>
	Font* LoadFont(const std::string& name, const std::string& ttfPath, float pixelHeight, uint32_t atlasSize = 1024);

	/// <summary>読み込み済みのフォントを引く。無ければ nullptr</summary>
	Font* Find(const std::string& name);

	/// <summary>フォント名を指定しなかったときに使うフォントを決める</summary>
	void SetDefaultFont(const std::string& name);
	Font* GetDefaultFont() const { return defaultFont_; }

	/// <summary>
	/// 全フォントのアトラスを GPU へ上げ直す。増えていなければ何もしない。
	/// アトラスを読む描画より前に、フレームに1回呼ぶ
	/// </summary>
	void FlushAtlases();

	void Finalize();

private:
	FontManager() = default;
	~FontManager() = default;
	FontManager(const FontManager&) = delete;
	FontManager& operator=(const FontManager&) = delete;

	std::unordered_map<std::string, std::unique_ptr<Font>> fonts_;
	Font* defaultFont_ = nullptr;
};
