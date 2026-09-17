#pragma once
#include <string>
#include <string_view>

/// <summary>
/// UTF-8 の文字列をコードポイントの並びへ直す。
///
/// ビルドは /utf-8 なので、ソースに直接書いた "日本語" はそのまま UTF-8 のバイト列になる。
/// 壊れたバイトは U+FFFD（下駄）に置き換えて読み飛ばすので、途中で止まることはない
/// </summary>
std::u32string DecodeUtf8(std::string_view text);
