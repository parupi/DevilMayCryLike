#include "GifLoader.h"
#include "TextureManager.h"
#include "Utility/StringUtility.h"
#include "Utility/Logger.h"
#include <wincodec.h>
#include <wrl/client.h>
#include <sstream>
#include <cassert>
#include <cstring>
#include <unordered_map>

using Microsoft::WRL::ComPtr;

// ロード済み GIF のキャッシュ (同一ファイルの 2 重ロードを防ぐ)
static std::unordered_map<std::string, GifInfo> s_cache;

namespace {

// PROPVARIANT から UINT を安全に読む。失敗時は defaultVal を返す。
UINT ReadUInt(IWICMetadataQueryReader* reader, const wchar_t* name, UINT defaultVal = 0)
{
    PROPVARIANT pv{};
    UINT result = defaultVal;
    if (SUCCEEDED(reader->GetMetadataByName(name, &pv))) {
        switch (pv.vt) {
        case VT_UI1: result = pv.bVal;  break;
        case VT_UI2: result = pv.uiVal; break;
        case VT_UI4: result = pv.ulVal; break;
        default: break;
        }
    }
    PropVariantClear(&pv);
    return result;
}

} // namespace

GifInfo GifLoader::Load(const std::string& fileName)
{
    // キャッシュヒット
    auto it = s_cache.find(fileName);
    if (it != s_cache.end()) {
        Logger::Log("[GifLoader] Cache hit: " + fileName + "\n");
        return it->second;
    }

    Logger::Log("[GifLoader] Loading: " + fileName + "\n");

    // ---- WIC factory -------------------------------------------------------
    ComPtr<IWICImagingFactory> factory;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory));
    ASSERT_MSG(SUCCEEDED(hr), "[GifLoader] WIC ImagingFactory の生成に失敗しました。");

    // ---- GIF を開く --------------------------------------------------------
    const std::string fullPath = "Resource/Images/" + fileName;
    std::wstring wpath = StringUtility::ConvertString(fullPath);
    ComPtr<IWICBitmapDecoder> decoder;
    hr = factory->CreateDecoderFromFilename(
        wpath.c_str(), nullptr, GENERIC_READ,
        WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr)) {
        std::ostringstream oss;
        oss << "[GifLoader] GIF ファイルを開けませんでした。\n"
            << "  パス: " << fullPath << "\n"
            << "  HRESULT: 0x" << std::hex << hr << "\n"
            << "  ※ファイルが存在するか、パスが正しいか確認してください。";
        ASSERT_MSG(false, oss.str().c_str());
    }

    // ---- グローバルメタデータ (キャンバスサイズ) --------------------------
    ComPtr<IWICMetadataQueryReader> globalMeta;
    hr = decoder->GetMetadataQueryReader(&globalMeta);
    ASSERT_MSG(SUCCEEDED(hr), "[GifLoader] GIF グローバルメタデータの取得に失敗しました。");

    UINT canvasW = ReadUInt(globalMeta.Get(), L"/logscrdesc/Width");
    UINT canvasH = ReadUInt(globalMeta.Get(), L"/logscrdesc/Height");
    {
        std::ostringstream oss;
        oss << "[GifLoader] GIF キャンバスサイズが不正です。\n"
            << "  ファイル: " << fileName << "\n"
            << "  Width=" << canvasW << ", Height=" << canvasH;
        ASSERT_MSG(canvasW > 0 && canvasH > 0, oss.str().c_str());
    }

    UINT frameCount = 0;
    decoder->GetFrameCount(&frameCount);
    {
        std::ostringstream oss;
        oss << "[GifLoader] GIF フレーム数が 0 です。\n"
            << "  ファイル: " << fileName;
        ASSERT_MSG(frameCount > 0, oss.str().c_str());
    }

    // ---- グリッドレイアウトを計算 (D3D12 最大テクスチャサイズ 16384px 対応) ---
    constexpr UINT kMaxTexDim = 16384u;
    UINT cols = (canvasW > 0) ? (std::min)(frameCount, kMaxTexDim / canvasW) : 1u;
    if (cols == 0) cols = 1;
    UINT rows = (frameCount + cols - 1) / cols;

    // ---- GifInfo を構築 ----------------------------------------------------
    GifInfo info;
    info.fileName     = fileName;
    info.frameCount   = static_cast<int>(frameCount);
    info.frameWidth   = static_cast<int>(canvasW);
    info.frameHeight  = static_cast<int>(canvasH);
    info.atlasColumns = static_cast<int>(cols);
    info.frameDurations.resize(frameCount);

    {
        std::ostringstream oss;
        oss << "[GifLoader] " << fileName
            << " | " << frameCount << " フレーム"
            << " | " << canvasW << "x" << canvasH << " px"
            << " | アトラス " << cols << " 列 x " << rows << " 行\n";
        Logger::Log(oss.str());
    }

    // ---- アトラスバッファ (グリッドレイアウト) ----------------------------
    const UINT atlasW = cols * canvasW;
    const UINT atlasH = rows * canvasH;
    std::vector<uint8_t> atlas(atlasW * atlasH * 4, 0);

    // 合成用キャンバス (RGBA)
    std::vector<uint8_t> canvas(canvasW * canvasH * 4, 0);
    std::vector<uint8_t> prevCanvas; // disposal == 3 用の退避バッファ

    for (UINT i = 0; i < frameCount; ++i) {
        // ---- フレーム取得 --------------------------------------------------
        ComPtr<IWICBitmapFrameDecode> frame;
        hr = decoder->GetFrame(i, &frame);
        if (FAILED(hr)) {
            std::ostringstream oss;
            oss << "[GifLoader] フレーム " << i << " の取得に失敗しました。\n"
                << "  ファイル: " << fileName
                << "  HRESULT: 0x" << std::hex << hr;
            ASSERT_MSG(false, oss.str().c_str());
        }

        ComPtr<IWICMetadataQueryReader> frameMeta;
        hr = frame->GetMetadataQueryReader(&frameMeta);
        if (FAILED(hr)) {
            std::ostringstream oss;
            oss << "[GifLoader] フレーム " << i << " のメタデータ取得に失敗しました。\n"
                << "  ファイル: " << fileName;
            Logger::Log(oss.str());
            // フレームメタデータ取得失敗はデフォルト値で続行
        }

        // フレーム遅延 (1/100 秒単位 → 秒)
        UINT delayCs = frameMeta ? ReadUInt(frameMeta.Get(), L"/grctlext/Delay", 10) : 10;
        info.frameDurations[i] = (delayCs == 0) ? 0.1f : static_cast<float>(delayCs) / 100.0f;

        // フレーム矩形
        UINT fLeft = frameMeta ? ReadUInt(frameMeta.Get(), L"/imgdesc/Left")             : 0;
        UINT fTop  = frameMeta ? ReadUInt(frameMeta.Get(), L"/imgdesc/Top")              : 0;
        UINT fW    = frameMeta ? ReadUInt(frameMeta.Get(), L"/imgdesc/Width",  canvasW)  : canvasW;
        UINT fH    = frameMeta ? ReadUInt(frameMeta.Get(), L"/imgdesc/Height", canvasH)  : canvasH;

        // 廃棄方法 (0/1=そのまま, 2=透明に戻す, 3=前フレームに戻す)
        UINT disposal = frameMeta ? ReadUInt(frameMeta.Get(), L"/grctlext/Disposal", 0) : 0;
        if (disposal == 3) prevCanvas = canvas;

        // ---- フレームを RGBA に変換 ----------------------------------------
        ComPtr<IWICFormatConverter> converter;
        hr = factory->CreateFormatConverter(&converter);
        if (FAILED(hr)) {
            std::ostringstream oss;
            oss << "[GifLoader] フレーム " << i << " : FormatConverter の生成に失敗しました。\n"
                << "  ファイル: " << fileName
                << "  HRESULT: 0x" << std::hex << hr;
            ASSERT_MSG(false, oss.str().c_str());
        }

        hr = converter->Initialize(
            frame.Get(), GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone, nullptr, 0.0f,
            WICBitmapPaletteTypeMedianCut);
        if (FAILED(hr)) {
            std::ostringstream oss;
            oss << "[GifLoader] フレーム " << i << " : RGBA 変換の初期化に失敗しました。\n"
                << "  ファイル: " << fileName
                << "  HRESULT: 0x" << std::hex << hr;
            ASSERT_MSG(false, oss.str().c_str());
        }

        std::vector<uint8_t> framePixels(fW * fH * 4);
        hr = converter->CopyPixels(
            nullptr, fW * 4,
            static_cast<UINT>(framePixels.size()), framePixels.data());
        if (FAILED(hr)) {
            std::ostringstream oss;
            oss << "[GifLoader] フレーム " << i << " : ピクセルデータのコピーに失敗しました。\n"
                << "  ファイル: " << fileName
                << "  HRESULT: 0x" << std::hex << hr;
            ASSERT_MSG(false, oss.str().c_str());
        }

        // ---- フレームをキャンバスに合成 (不透明ピクセルのみ上書き) --------
        for (UINT y = 0; y < fH; ++y) {
            for (UINT x = 0; x < fW; ++x) {
                UINT cx = fLeft + x;
                UINT cy = fTop  + y;
                if (cx >= canvasW || cy >= canvasH) continue;
                uint32_t src = (y * fW + x) * 4;
                uint32_t dst = (cy * canvasW + cx) * 4;
                if (framePixels[src + 3] > 0) {
                    canvas[dst]     = framePixels[src];
                    canvas[dst + 1] = framePixels[src + 1];
                    canvas[dst + 2] = framePixels[src + 2];
                    canvas[dst + 3] = framePixels[src + 3];
                }
            }
        }

        // ---- キャンバスのスナップショットをアトラスに書き込む (グリッド) --
        UINT col     = i % cols;
        UINT row     = i / cols;
        UINT offsetX = col * canvasW;
        UINT offsetY = row * canvasH;
        for (UINT y = 0; y < canvasH; ++y) {
            std::memcpy(
                atlas.data() + ((offsetY + y) * atlasW + offsetX) * 4,
                canvas.data() + y * canvasW * 4,
                canvasW * 4);
        }

        // ---- 廃棄処理 ------------------------------------------------------
        if (disposal == 2) {
            for (UINT y = fTop; y < fTop + fH && y < canvasH; ++y)
                for (UINT x = fLeft; x < fLeft + fW && x < canvasW; ++x) {
                    UINT idx = (y * canvasW + x) * 4;
                    canvas[idx] = canvas[idx+1] = canvas[idx+2] = canvas[idx+3] = 0;
                }
        } else if (disposal == 3 && !prevCanvas.empty()) {
            canvas = std::move(prevCanvas);
            prevCanvas.clear();
        }
    }

    // ---- アトラスを TextureManager に登録 ----------------------------------
    TextureManager::GetInstance().LoadTextureFromMemory(
        fileName, atlas.data(), atlasW, atlasH);

    s_cache[fileName] = info;

    Logger::Log("[GifLoader] 完了: " + fileName + "\n");
    return info;
}

void GifLoader::ClearCache()
{
    s_cache.clear();
    Logger::Log("[GifLoader] キャッシュをクリアしました。\n");
}
