
# すべての Engine cpp で namespace Guchis { の位置を正しく修正する
# - 既存の namespace Guchis { ... } // namespace Guchis を除去
# - 全ディレクティブ (#include, #ifdef 等) が終わった後に再挿入

param([string]$NS = "Guchis")

$NS_OPEN  = "namespace $NS {"
$NS_CLOSE = "} // namespace $NS"
$root = "C:\Users\kawah\source\repos\DevilMayCryLike\Engine"

function Find-PreambleEnd([string[]]$lines) {
    $lastDir = -1
    $depth   = 0

    for ($i = 0; $i -lt $lines.Length; $i++) {
        $t = $lines[$i].Trim()

        # 空行・コメントは無視（プリアンブルを終わらせない）
        if ([string]::IsNullOrWhiteSpace($t) -or
            $t.StartsWith("//") -or $t.StartsWith("/*") -or
            $t -match '^\*') { continue }

        # ディレクティブ
        if ($t -match '^#\s*(include|pragma)\b')        { $lastDir = $i; continue }
        if ($t -match '^#\s*if(def|ndef)?\b')           { $depth++; $lastDir = $i; continue }
        if ($t -match '^#\s*(else|elif)\b')             { $lastDir = $i; continue }
        if ($t -match '^#\s*endif\b')                   { $depth--; $lastDir = $i; continue }
        if ($t -match '^#\s*define\b')                  { $lastDir = $i; continue }

        # namespace Guchis { はスキップ（後で置き直す）
        if ($t -eq $NS_OPEN)                            { continue }

        # 深さ > 0 の間はコード行でもプリアンブル扱い（#ifdef内）
        if ($depth -gt 0) { $lastDir = $i; continue }

        # 深さ = 0 でコード行 → プリアンブル終了
        break
    }
    return $lastDir
}

Get-ChildItem -Path $root -Recurse -Filter "*.cpp" | ForEach-Object {
    $path = $_.FullName
    $raw  = [System.IO.File]::ReadAllText($path, [System.Text.Encoding]::UTF8)
    $text = $raw -replace "`r`n","`n" -replace "`r","`n"

    # namespace Guchis { が存在しなければスキップ
    if ($text -notmatch [regex]::Escape($NS_OPEN)) { return }

    $lines = $text -split "`n"

    # ─────────────────────────────────────────────────
    # 1. 既存の `namespace Guchis {` 行を削除（前後の空行も1つ）
    # ─────────────────────────────────────────────────
    $newLines = [System.Collections.Generic.List[string]]($lines)
    $idx = -1
    for ($i = 0; $i -lt $newLines.Count; $i++) {
        if ($newLines[$i].Trim() -eq $NS_OPEN) { $idx = $i; break }
    }
    if ($idx -lt 0) { return }

    $newLines.RemoveAt($idx)
    # 直後の空行を削除
    if ($idx -lt $newLines.Count -and [string]::IsNullOrWhiteSpace($newLines[$idx])) {
        $newLines.RemoveAt($idx)
    }
    # 直前の空行を削除
    if ($idx -gt 0 -and [string]::IsNullOrWhiteSpace($newLines[$idx-1])) {
        $newLines.RemoveAt($idx-1)
    }

    # ─────────────────────────────────────────────────
    # 2. 正しい挿入位置を計算
    # ─────────────────────────────────────────────────
    $arr = $newLines.ToArray()
    $insertAfter = Find-PreambleEnd $arr

    $insertAt = $insertAfter + 1

    # 直前が空行でなければ空行を挿入
    if ($insertAt -gt 0 -and -not [string]::IsNullOrWhiteSpace($newLines[$insertAt-1])) {
        $newLines.Insert($insertAt, "")
        $insertAt++
    }
    $newLines.Insert($insertAt, $NS_OPEN)

    $newText = ($newLines -join "`n")
    [System.IO.File]::WriteAllText($path, $newText, [System.Text.Encoding]::UTF8)
    Write-Host "FIXED: $($_.Name)"
}

Write-Host "`n=== 完了 ==="
