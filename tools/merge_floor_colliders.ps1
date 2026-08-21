<#
.SYNOPSIS
    ステージの床タイルのコライダーを、隣接するタイルごとに大きな箱へまとめ直す。

.DESCRIPTION
    床が 6m のタイル 100枚以上で敷き詰められていると、タイルの継ぎ目ごとに
    「押し出し方向が横を向く」「2〜4枚に同時接触して押し出しが多重にかかる」
    という当たり判定のバグが出る（継ぎ目で引っかかる / 敵が接地で上下に跳ねる）。

    このスクリプトは同じ高さで隣接している床タイルを長方形に切り分け、
    長方形ごとに 1つの大きな OBB コライダーへまとめる。
    コライダーは長方形の中心に一番近いタイルへ持たせ（offset でずらす）、
    残りのタイルからは collider を取り除く。見た目のタイルは一切動かさない。

    床タイルのコライダーは毎回まるごと作り直す。個別に調整していても
    再実行で標準の箱に戻るので注意（調整が必要なら Ground を別に置くこと）。
    ステージをエディタで編集して床を足し引きしたら、その都度実行し直す。

.PARAMETER StagePath
    対象のステージ json。既定は Resource/Stage/Stage.json。

.PARAMETER FloorModel
    床として扱うモデル名。既定は ModularFloor。

.PARAMETER DryRun
    ファイルを書き換えず、結合結果だけを表示する。

.EXAMPLE
    pwsh -File tools/merge_floor_colliders.ps1
    pwsh -File tools/merge_floor_colliders.ps1 -DryRun
#>
[CmdletBinding()]
param(
    [string]$StagePath = 'Resource/Stage/Stage.json',
    [string]$FloorModel = 'ModularFloor',
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

# --- 床タイル1枚あたりのセルサイズ(m)。ワールド換算の halfExtents から求める ---
function Get-WorldHalf($obj) {
    $he = $obj.collider.halfExtents
    $s = $obj.scale
    return @([math]::Abs($he[0] * $s[0]), [math]::Abs($he[1] * $s[1]), [math]::Abs($he[2] * $s[2]))
}

function Test-IdentityRotation($rot) {
    # クォータニオン (x,y,z,w) が無回転か
    return ([math]::Abs($rot[0]) -lt 1e-4) -and ([math]::Abs($rot[1]) -lt 1e-4) -and
           ([math]::Abs($rot[2]) -lt 1e-4) -and ([math]::Abs([math]::Abs($rot[3]) - 1.0) -lt 1e-4)
}

# ヒストグラム法で、空きセルだけからなる最大面積の長方形を求める。
# grid は [row][col] の bool 配列。戻りは @{ r0; r1; c0; c1; area }
function Get-LargestRectangle($grid, $rows, $cols) {
    $heights = New-Object 'int[]' $cols
    $best = @{ area = 0; r0 = 0; r1 = -1; c0 = 0; c1 = -1 }

    for ($r = 0; $r -lt $rows; $r++) {
        for ($c = 0; $c -lt $cols; $c++) {
            $heights[$c] = if ($grid[$r][$c]) { $heights[$c] + 1 } else { 0 }
        }

        # (開始列, 高さ) のスタック。末尾に番兵として高さ0を流し込む
        $stack = [System.Collections.Generic.List[int[]]]::new()
        for ($c = 0; $c -le $cols; $c++) {
            $h = if ($c -lt $cols) { $heights[$c] } else { 0 }
            $start = $c
            while ($stack.Count -gt 0 -and $stack[$stack.Count - 1][1] -gt $h) {
                $top = $stack[$stack.Count - 1]
                $stack.RemoveAt($stack.Count - 1)
                $sc = $top[0]
                $sh = $top[1]
                $area = $sh * ($c - $sc)
                if ($area -gt $best.area) {
                    $best = @{ area = $area; r0 = $r - $sh + 1; r1 = $r; c0 = $sc; c1 = $c - 1 }
                }
                $start = $sc
            }
            if ($h -gt 0) { $stack.Add(@($start, $h)) }
        }
    }
    return $best
}

# --- 読み込み ---
if (-not (Test-Path $StagePath)) { throw "ステージが見つかりません: $StagePath" }
$raw = Get-Content -Raw -Encoding UTF8 $StagePath
$stage = $raw | ConvertFrom-Json

$floors = @($stage.objects | Where-Object { $_.model -eq $FloorModel -and $_.class -eq 'Ground' })
if ($floors.Count -eq 0) { throw "床タイル ($FloorModel) が1枚も見つかりません" }

Write-Host "ステージ: $StagePath"
Write-Host "床タイル: $($floors.Count) 枚"

# --- 標準サイズ（最頻値）を決め、そこから外れるタイルは触らない ---
$sizeGroups = $floors | Where-Object { $_.collider } | Group-Object {
    $h = Get-WorldHalf $_
    "{0},{1},{2}" -f [math]::Round($h[0], 4), [math]::Round($h[1], 4), [math]::Round($h[2], 4)
} | Sort-Object Count -Descending

if ($sizeGroups.Count -eq 0) { throw "床タイルにコライダーが1つも付いていないため、標準サイズを判定できません" }
$standard = $sizeGroups[0].Name.Split(',') | ForEach-Object { [double]$_ }
$cellX = $standard[0] * 2
$cellZ = $standard[2] * 2
Write-Host ("床1枚のコライダー: {0} x {1} x {2} (セル {3}m x {4}m)" -f ($standard[0]*2), ($standard[1]*2), ($standard[2]*2), $cellX, $cellZ)

# --- 結合対象を選ぶ（無回転・標準サイズ・グリッド上）---
$targets = @()
$skipped = @()
foreach ($t in $floors) {
    if (-not (Test-IdentityRotation $t.rotate)) { $skipped += "$($t.name): 回転しているため対象外"; continue }
    if ($t.collider) {
        $h = Get-WorldHalf $t
        # 厚み(Y)だけは個体差を許さない。XZ は結合済みの大きな箱かもしれないので見ない
        if ([math]::Abs($h[1] - $standard[1]) -gt 1e-3) { $skipped += "$($t.name): 厚みが標準と違うため対象外"; continue }
    }
    $targets += $t
}
foreach ($s in $skipped) { Write-Host "  スキップ: $s" -ForegroundColor Yellow }

# --- 高さごとにグリッドへ並べる ---
$levels = @{}
foreach ($t in $targets) {
    $key = [math]::Round($t.translate[1], 4)
    if (-not $levels.ContainsKey($key)) { $levels[$key] = @() }
    $levels[$key] += $t
}

$rects = @()
foreach ($levelY in ($levels.Keys | Sort-Object)) {
    $tiles = $levels[$levelY]
    $xs = $tiles | ForEach-Object { $_.translate[0] }
    $zs = $tiles | ForEach-Object { $_.translate[2] }
    $minX = ($xs | Measure-Object -Minimum).Minimum
    $minZ = ($zs | Measure-Object -Minimum).Minimum
    $maxX = ($xs | Measure-Object -Maximum).Maximum
    $maxZ = ($zs | Measure-Object -Maximum).Maximum
    $cols = [int][math]::Round(($maxX - $minX) / $cellX) + 1
    $rows = [int][math]::Round(($maxZ - $minZ) / $cellZ) + 1

    # セル -> タイル。グリッドに乗らないタイルはその場に単独で残す
    $cell = @{}
    $offGrid = @()
    foreach ($t in $tiles) {
        $c = ($t.translate[0] - $minX) / $cellX
        $r = ($t.translate[2] - $minZ) / $cellZ
        if ([math]::Abs($c - [math]::Round($c)) -gt 1e-3 -or [math]::Abs($r - [math]::Round($r)) -gt 1e-3) {
            $offGrid += $t
            continue
        }
        $key = "{0},{1}" -f [int][math]::Round($r), [int][math]::Round($c)
        if ($cell.ContainsKey($key)) { $offGrid += $t; continue }  # 同じセルの重複タイル
        $cell[$key] = $t
    }
    foreach ($t in $offGrid) { Write-Host "  グリッド外: $($t.name) は単独のコライダーのままにします" -ForegroundColor Yellow }

    $grid = New-Object 'object[]' $rows
    for ($r = 0; $r -lt $rows; $r++) {
        $line = New-Object 'bool[]' $cols
        for ($c = 0; $c -lt $cols; $c++) { $line[$c] = $cell.ContainsKey("$r,$c") }
        $grid[$r] = $line
    }

    # 空きが無くなるまで最大長方形を切り出す
    while ($true) {
        $best = Get-LargestRectangle $grid $rows $cols
        if ($best.area -le 0) { break }
        for ($r = $best.r0; $r -le $best.r1; $r++) {
            for ($c = $best.c0; $c -le $best.c1; $c++) { $grid[$r][$c] = $false }
        }
        $members = @()
        for ($r = $best.r0; $r -le $best.r1; $r++) {
            for ($c = $best.c0; $c -le $best.c1; $c++) { $members += $cell["$r,$c"] }
        }
        $rects += [pscustomobject]@{
            levelY  = $levelY
            centerX = $minX + (($best.c0 + $best.c1) / 2.0) * $cellX
            centerZ = $minZ + (($best.r0 + $best.r1) / 2.0) * $cellZ
            halfX   = (($best.c1 - $best.c0 + 1) * $cellX) / 2.0
            halfZ   = (($best.r1 - $best.r0 + 1) * $cellZ) / 2.0
            members = $members
        }
    }

    foreach ($t in $offGrid) {
        $rects += [pscustomobject]@{
            levelY = $levelY; centerX = $t.translate[0]; centerZ = $t.translate[2]
            halfX = $standard[0]; halfZ = $standard[2]; members = @($t)
        }
    }
}

# --- 反映 ---
$colliderCountBefore = @($floors | Where-Object { $_.collider }).Count
foreach ($rect in $rects) {
    # 長方形の中心に一番近いタイルにコライダーを持たせる
    $carrier = $rect.members | Sort-Object {
        [math]::Pow($_.translate[0] - $rect.centerX, 2) + [math]::Pow($_.translate[2] - $rect.centerZ, 2)
    } | Select-Object -First 1

    foreach ($t in $rect.members) {
        if ($t -eq $carrier) { continue }
        if ($t.PSObject.Properties['collider']) { $t.PSObject.Properties.Remove('collider') }
    }

    # halfExtents はオーナーのスケールが掛かる。offset は掛からない（ワールド長）
    # PowerShell はカンマの結合が除算より強いので、要素ごとに括弧で囲むこと
    $half = @(
        ($rect.halfX / $carrier.scale[0]),
        ($standard[1] / $carrier.scale[1]),
        ($rect.halfZ / $carrier.scale[2])
    )
    $offset = @(
        ($rect.centerX - $carrier.translate[0]),
        0.0,
        ($rect.centerZ - $carrier.translate[2])
    )
    $collider = [pscustomobject]@{ shape = 'OBB'; offset = $offset; halfExtents = $half }
    if ($carrier.PSObject.Properties['collider']) {
        $carrier.collider = $collider
    } else {
        $carrier | Add-Member -NotePropertyName collider -NotePropertyValue $collider
    }
}

Write-Host ""
Write-Host "結合結果:"
foreach ($rect in ($rects | Sort-Object levelY, centerX, centerZ)) {
    $carrier = $rect.members | Sort-Object {
        [math]::Pow($_.translate[0] - $rect.centerX, 2) + [math]::Pow($_.translate[2] - $rect.centerZ, 2)
    } | Select-Object -First 1
    Write-Host ("  y={0,-6} 中心({1,7:0.##},{2,7:0.##}) {3,5:0.##}m x {4,5:0.##}m  タイル{5,3}枚 -> {6}" -f `
        $rect.levelY, $rect.centerX, $rect.centerZ, ($rect.halfX * 2), ($rect.halfZ * 2), $rect.members.Count, $carrier.name)
}
Write-Host ""
Write-Host ("床コライダー: {0} 個 -> {1} 個" -f $colliderCountBefore, $rects.Count) -ForegroundColor Green

if ($DryRun) {
    Write-Host "(DryRun のためファイルは変更していません)" -ForegroundColor Yellow
    return
}

# --- バックアップして書き出し ---
$backupDir = Join-Path (Split-Path -Parent $StagePath) '_backup'
if (-not (Test-Path $backupDir)) { New-Item -ItemType Directory $backupDir | Out-Null }
$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$backup = Join-Path $backupDir ("{0}_beforeMerge_{1}.json" -f [System.IO.Path]::GetFileNameWithoutExtension($StagePath), $stamp)
Copy-Item $StagePath $backup
Write-Host "バックアップ: $backup"

$json = $stage | ConvertTo-Json -Depth 20
# BOM無しUTF8で書く（nlohmann は BOM を読めない）
[System.IO.File]::WriteAllText((Resolve-Path $StagePath), $json, (New-Object System.Text.UTF8Encoding($false)))
Write-Host "書き出し完了: $StagePath" -ForegroundColor Green
