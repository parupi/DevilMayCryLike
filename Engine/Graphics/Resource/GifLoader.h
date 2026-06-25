#pragma once
#include <string>
#include <vector>

// GIF 1 枚分の情報。AnimatedSprite がフレーム制御に使う。
struct GifInfo {
    std::string fileName;           // TextureManager へ渡す相対パス (例: "UI/anim.gif")
    int         frameCount   = 0;
    int         frameWidth   = 0;   // 1 フレームの幅 (px)
    int         frameHeight  = 0;   // 1 フレームの高さ (px)
    int         atlasColumns = 0;   // アトラス内の列数 (グリッドレイアウト対応)
    std::vector<float> frameDurations; // 各フレームの表示時間 (秒)
};

class GifLoader
{
public:
    // GIF を読み込んでスプライトシートを TextureManager に登録し、情報を返す。
    // fileName: "Resource/Images/" からの相対パス (例: "UI/animation.gif")
    // 同一 fileName は 2 度目以降ロードをスキップして返す。
    static GifInfo Load(const std::string& fileName);

    // ロード済みキャッシュを解放する。SpriteManager::Finalize() から呼ぶこと。
    static void ClearCache();

private:
    GifLoader() = delete;
};
