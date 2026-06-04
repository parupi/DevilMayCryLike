
# ============================================================
# Engine ファイルを namespace Guchis で包む
# App ヘッダに後方互換 using 宣言を追加（App コード変更なし）
# ============================================================
param(
    [string]$Root = "C:\Users\kawah\source\repos\DevilMayCryLike",
    [string]$NS   = "Guchis"
)

$engineDir = Join-Path $Root "Engine"
$NS_OPEN   = "namespace $NS {"
$NS_CLOSE  = "} // namespace $NS"

# ──────────────────────────────────────────────
# ヘルパー: プリアンブル末尾行のインデックスを返す
# (最後の #include / #pragma once の行)
# ──────────────────────────────────────────────
function Get-PreambleEnd([string[]]$lines) {
    $last = -1
    foreach ($i in 0..([Math]::Min($lines.Length, 200) - 1)) {
        $t = $lines[$i].TrimEnd()
        if ($t -match '^#\s*(include|pragma once)') {
            $last = $i
        } elseif ($t -match '^[^#\s/]' -and $last -ge 0) {
            break  # 最初のコード行に到達
        }
    }
    return $last
}

# ──────────────────────────────────────────────
# ヘルパー: 後方互換 using を生成する
# ──────────────────────────────────────────────
function Get-UsingDecls([string[]]$codeLines, [string]$ns) {
    $names = [System.Collections.Generic.HashSet[string]]::new()
    foreach ($line in $codeLines) {
        $t = $line.Trim()
        # class / struct / enum class の名前
        if ($t -match '^(class|struct|enum\s+class)\s+([A-Za-z_][A-Za-z0-9_]*)(\s|:|{|;|$)') {
            $null = $names.Add($Matches[2])
        }
        # using Alias = ...
        if ($t -match '^using\s+([A-Za-z_][A-Za-z0-9_]*)\s*=') {
            $null = $names.Add($Matches[1])
        }
        # static const / constexpr (定数 → using宣言不可なので namespace using 経由)
    }
    return ($names | Sort-Object | ForEach-Object { "using ${ns}::$_;" })
}

# ──────────────────────────────────────────────
# Engine ヘッダ処理
# ──────────────────────────────────────────────
Get-ChildItem -Path $engineDir -Recurse -Filter "*.h" | ForEach-Object {
    $path    = $_.FullName
    $raw     = [System.IO.File]::ReadAllText($path, [System.Text.Encoding]::UTF8)

    # すでに処理済みならスキップ
    if ($raw -match [regex]::Escape($NS_OPEN)) {
        Write-Host "SKIP  $path"
        return
    }

    # CRLF → LF 正規化してから分割
    $text  = $raw -replace "`r`n", "`n" -replace "`r", "`n"
    $lines = $text -split "`n"

    $end   = Get-PreambleEnd $lines

    if ($end -lt 0) {
        # include が 1 つもない場合：#pragma once の直後を挿入点とする
        $pragmaIdx = ($lines | Select-String -SimpleMatch '#pragma once').LineNumber
        $end = if ($pragmaIdx) { $pragmaIdx - 1 } else { -1 }
    }

    $preamble  = ($end -ge 0) ? ($lines[0..$end] -join "`n") : ""
    $codeBlock = ($end -ge 0) ? (($lines[($end+1)..($lines.Length-1)] -join "`n").TrimStart()) : $text.TrimStart()

    $usings = Get-UsingDecls ($codeBlock -split "`n") $NS

    $newText = if ($preamble) {
        $preamble + "`n`n" + $NS_OPEN + "`n" + $codeBlock + "`n" + $NS_CLOSE + "`n"
    } else {
        $NS_OPEN + "`n" + $codeBlock + "`n" + $NS_CLOSE + "`n"
    }

    if ($usings) {
        $newText += "`n" + ($usings -join "`n") + "`n"
    }

    [System.IO.File]::WriteAllText($path, $newText, [System.Text.Encoding]::UTF8)
    Write-Host "OK    $path"
}

# ──────────────────────────────────────────────
# Engine cpp 処理
# ──────────────────────────────────────────────
Get-ChildItem -Path $engineDir -Recurse -Filter "*.cpp" | ForEach-Object {
    $path = $_.FullName
    $raw  = [System.IO.File]::ReadAllText($path, [System.Text.Encoding]::UTF8)

    if ($raw -match [regex]::Escape($NS_OPEN)) {
        Write-Host "SKIP  $path"
        return
    }

    $text  = $raw -replace "`r`n", "`n" -replace "`r", "`n"
    $lines = $text -split "`n"
    $end   = Get-PreambleEnd $lines

    $preamble  = ($end -ge 0) ? ($lines[0..$end] -join "`n") : ""
    $codeBlock = ($end -ge 0) ? (($lines[($end+1)..($lines.Length-1)] -join "`n").TrimStart()) : $text.TrimStart()

    $newText = if ($preamble) {
        $preamble + "`n`n" + $NS_OPEN + "`n" + $codeBlock + "`n" + $NS_CLOSE + "`n"
    } else {
        $NS_OPEN + "`n" + $codeBlock + "`n" + $NS_CLOSE + "`n"
    }

    [System.IO.File]::WriteAllText($path, $newText, [System.Text.Encoding]::UTF8)
    Write-Host "OK    $path"
}

Write-Host "`n=== 完了 ==="
