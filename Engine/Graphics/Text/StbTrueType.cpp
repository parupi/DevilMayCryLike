// stb_truetype の実体は、この翻訳単位だけでコンパイルする。
//
// ヘッダは ImGui が同梱しているものを借りている（Externals/imgui/imstb_truetype.h。中身は stb_truetype 1.26）。
// ImGui 側は imgui_draw.cpp の中で namespace ImStb ＋ STBTT_STATIC を付けて取り込んでいるので、
// こちらが素の名前で実体を持ってもシンボルはぶつからない。
//
// 上流のコードなので警告は止めておく（premake5.lua の warnings "Extra" と、
// 自前コードの警告0を保つ方針のため。Externals/** の警告は premake 側で切ってあるが、
// このファイルは Engine/ にあるので個別に囲う）
#pragma warning(push, 0)
#define STB_TRUETYPE_IMPLEMENTATION
#include <imstb_truetype.h>
#pragma warning(pop)
