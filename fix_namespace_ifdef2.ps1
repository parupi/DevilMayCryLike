
# 全 Engine cpp の namespace 挿入位置を修正:
# - すべての #include（#ifdef _DEBUG ブロック内も含む）の後に namespace を置く
# - まずすでにある namespace Guchis { を削除し、正しい位置に再挿入する

param([string]$NS = "Guchis")

$NS_OPEN  = "namespace $NS {"
$NS_CLOSE = "} // namespace $NS"
$root = "C:\Users\kawah\source\repos\DevilMayCryLike\Engine"

Get-ChildItem -Path $root -Recurse -Filter "*.cpp" | ForEach-Object {
    $path = $_.FullName
    $raw  = [System.IO.File]::ReadAllText($path, [System.Text.Encoding]::UTF8)
    $text = $raw -replace "`r`n", "`n" -replace "`r", "`n"
    $lines = $text -split "`n"

    # namespace Guchis { が存在するか
    $nsIdx = -1
    for ($i = 0; $i -lt $lines.Length; $i++) {
        if ($lines[$i].Trim() -eq $NS_OPEN) { $nsIdx = $i; break }
    }
    if ($nsIdx -lt 0) { return }  # このファイルはスキップ

    # ──────────────────────────────────────────────────────
    # 「すべての include / #pragma / #ifdef/#endif ブロック」が終わる行を探す
    # = 最初の非ディレクティブ・非空白・非コメント行の直前
    # ──────────────────────────────────────────────────────
    $lastPrelude = -1
    $inIfBlock = $false
    $ifDepth = 0

    for ($i = 0; $i -lt $lines.Length; $i++) {
        $t = $lines[$i].Trim()

        # 完全空行 or コメント行は通過
        if ([string]::IsNullOrWhiteSpace($t) -or $t.StartsWith("//") -or $t.StartsWith("/*") -or $t.StartsWith("*")) {
            continue
        }

        # #pragma once / #include → プリアンブル行
        if ($t -match '^#\s*(pragma|include)\b') {
            $lastPrelude = $i
            continue
        }

        # #ifdef / #ifndef / #if → 深さ増加
        if ($t -match '^#\s*if(def|ndef)?\b') {
            $ifDepth++
            $lastPrelude = $i
            continue
        }

        # #else / #elif → プリアンブル行（フラットな if にも対応）
        if ($t -match '^#\s*(else|elif)\b') {
            $lastPrelude = $i
            continue
        }

        # #endif → 深さ減少
        if ($t -match '^#\s*endif\b') {
            $ifDepth--
            $lastPrelude = $i
            # 深さが 0 に戻ったらそのブロックを通り抜けた
            continue
        }

        # それ以外のコード行 → プリアンブル終了
        if ($ifDepth -eq 0) {
            break
        } else {
            # #ifdef 内のコード行はまだプリアンブルの一部として無視
            $lastPrelude = $i
        }
    }

    # すでに正しい位置にあれば何もしない
    if ($nsIdx -eq $lastPrelude + 1 -or
        ($nsIdx -gt 0 -and [string]::IsNullOrWhiteSpace($lines[$nsIdx - 1]) -and $nsIdx - 1 -eq $lastPrelude + 1)) {
        # 位置が正しい (空行1行分の誤差は許容)
        return
    }

    # ──────────────────────────────────────────────────────
    # 1. 既存の `namespace Guchis {` 行（と前後の空行）を削除
    # ──────────────────────────────────────────────────────
    $newLines = [System.Collections.Generic.List[string]]$lines

    # 削除前に nsIdx を再確認
    $actualNsIdx = -1
    for ($i = 0; $i -lt $newLines.Count; $i++) {
        if ($newLines[$i].Trim() -eq $NS_OPEN) { $actualNsIdx = $i; break }
    }
    if ($actualNsIdx -lt 0) { return }

    $newLines.RemoveAt($actualNsIdx)
    # 前後の余分な空行を1行分削除
    if ($actualNsIdx -lt $newLines.Count -and [string]::IsNullOrWhiteSpace($newLines[$actualNsIdx])) {
        $newLines.RemoveAt($actualNsIdx)
    }
    $checkPrev = $actualNsIdx - 1
    if ($checkPrev -ge 0 -and [string]::IsNullOrWhiteSpace($newLines[$checkPrev])) {
        $newLines.RemoveAt($checkPrev)
        $actualNsIdx--
    }

    # ──────────────────────────────────────────────────────
    # 2. 正しい位置に再挿入: lastPrelude の次の行
    #    (削除によってインデックスがずれているので再計算)
    # ──────────────────────────────────────────────────────
    # lastPrelude は削除前のインデックスなので、
    # 削除した行が lastPrelude より前かどうかで調整
    $adjust = if ($actualNsIdx -le $lastPrelude) { 1 } else { 0 }
    $insertAt = $lastPrelude - $adjust + 1

    # 挿入前に空行を1つ追加（前の行が空行でなければ）
    if ($insertAt -gt 0 -and -not [string]::IsNullOrWhiteSpace($newLines[$insertAt - 1])) {
        $newLines.Insert($insertAt, "")
        $insertAt++
    }
    $newLines.Insert($insertAt, $NS_OPEN)

    $newText = ($newLines -join "`n")
    [System.IO.File]::WriteAllText($path, $newText, [System.Text.Encoding]::UTF8)
    Write-Host "FIXED: $path"
}

Write-Host "`n=== 完了 ==="
