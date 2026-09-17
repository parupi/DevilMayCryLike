<#
.SYNOPSIS
  チュートリアル用GIFを「画面に出るサイズ・フレームレート」まで落とす。

.DESCRIPTION
  GifLoader は GIF の全フレームを非圧縮RGBA8の1枚のアトラステクスチャに展開する
  （Engine/Graphics/Rendering/Resource/GifLoader.cpp）。
  そのためVRAM使用量は「フレーム数 x 幅 x 高さ x 4byte」で決まり、
  GIFファイル自体の圧縮サイズはまったく効かない。

  元素材は約55fpsの画面キャプチャで、755x554 や 556x381 の解像度がある。
  一方 Tutorial.cpp が実際に描画するのは 350x240 px。
  つまり必要な5倍以上のピクセルを、必要な倍以上のフレーム数で積んでいた。
  ここで表示サイズ・20fpsに落とすと、見た目を変えずにVRAMが約8分の1になる。

  Width/Height の既定値は App/Tutorial/Tutorial.cpp の
  tutorialImage->GetSprite()->SetSize({350.0f, 240.0f}) と対応している。
  表示サイズを変えたらこの値も合わせて再変換すること。

.NOTES
  ffmpeg が必要。PATH に無い場合は -FFmpeg で明示的にパスを渡す。
  元のGITは git に入っているので、失敗しても
  `git checkout -- Resource/Images/Tutorial/` で戻せる。

.EXAMPLE
  pwsh -File tools/shrink_tutorial_gifs.ps1
  pwsh -File tools/shrink_tutorial_gifs.ps1 -Fps 15 -WhatIfOnly
#>
[CmdletBinding()]
param(
    # 出力解像度。Tutorial.cpp の表示サイズに合わせる
    [int]$Width = 350,
    [int]$Height = 240,

    # 出力フレームレート。GIFの遅延は1/100秒単位なので、100の約数だと誤差が出ない
    # (20fps -> 5cs, 12.5fps -> 8cs, 10fps -> 10cs)
    [double]$Fps = 20,

    [string]$InputDir = "Resource/Images/Tutorial",

    [string]$FFmpeg = "ffmpeg",

    # 変換せず、削減見込みだけ表示する
    [switch]$WhatIfOnly
)

$ErrorActionPreference = "Stop"

if (-not (Get-Command $FFmpeg -ErrorAction SilentlyContinue)) {
    throw "ffmpeg が見つかりません: '$FFmpeg' — -FFmpeg <path> で明示してください。"
}
if (-not (Test-Path $InputDir)) {
    throw "入力ディレクトリが見つかりません: $InputDir"
}

Add-Type -AssemblyName System.Drawing

# GifLoader と同じ式でアトラスのVRAM使用量[MB]を求める
function Get-AtlasMB {
    param([int]$FrameW, [int]$FrameH, [int]$Frames)
    $cols = [Math]::Min($Frames, [Math]::Floor(16384 / $FrameW))
    if ($cols -lt 1) { $cols = 1 }
    $rows = [Math]::Ceiling($Frames / $cols)
    return ($cols * $FrameW * $rows * $FrameH * 4) / 1MB
}

function Get-GifStats {
    param([string]$Path)
    $img = [System.Drawing.Image]::FromFile((Resolve-Path $Path))
    try {
        $dim = New-Object System.Drawing.Imaging.FrameDimension $img.FrameDimensionsList[0]
        $frames = $img.GetFrameCount($dim)
        # PropertyTagFrameDelay (0x5100): フレームごとの遅延 (1/100秒) の配列。
        # GifLoader / AnimatedSprite と同じ解釈で尺を出す
        # (GifLoader.cpp: delay 0 のみ 0.1秒。1cs はそのまま 0.01秒 = 100fps 扱い)
        $seconds = 0.0
        try {
            $prop = $img.GetPropertyItem(0x5100)
            for ($i = 0; $i -lt $frames; $i++) {
                $cs = [BitConverter]::ToInt32($prop.Value, $i * 4)
                $seconds += if ($cs -eq 0) { 0.1 } else { $cs / 100.0 }
            }
        } catch { $seconds = $frames * 0.1 }
        return [pscustomobject]@{
            Width    = $img.Width
            Height   = $img.Height
            Frames   = $frames
            Seconds  = $seconds
            AtlasMB  = Get-AtlasMB -FrameW $img.Width -FrameH $img.Height -Frames $frames
            FileMB   = (Get-Item $Path).Length / 1MB
        }
    } finally { $img.Dispose() }
}

$gifs = Get-ChildItem $InputDir -Filter *.gif | Sort-Object Name
if ($gifs.Count -eq 0) { throw "GIFが見つかりません: $InputDir" }

$beforeTotal = 0.0
$afterTotal = 0.0
$results = @()

foreach ($gif in $gifs) {
    $before = Get-GifStats $gif.FullName

    if ($WhatIfOnly) {
        # 尺から出力フレーム数を見積もる
        $estFrames = [Math]::Max(1, [Math]::Round($before.Seconds * $Fps))
        $estMB = Get-AtlasMB -FrameW $Width -FrameH $Height -Frames $estFrames
        $beforeTotal += $before.AtlasMB
        $afterTotal += $estMB
        $results += [pscustomobject]@{
            Name = $gif.Name; BeforeMB = [Math]::Round($before.AtlasMB, 1)
            AfterMB = [Math]::Round($estMB, 1); Frames = "$($before.Frames) -> ~$estFrames"
        }
        continue
    }

    $tmp = Join-Path $gif.DirectoryName ("_tmp_" + $gif.Name)

    # 1パスでパレット生成と適用を行う (split で同じ入力を2系統に分ける)。
    # - scale の lanczos は縮小時に輪郭が最も素直
    # - palettegen stats_mode=full … 全フレームから1つのグローバルパレットを作る
    # - dither=bayer … 誤差拡散と違いフレーム間でパターンが暴れないので、
    #                  差分フレームが小さくなり、目にもチラつかない
    $filter = "fps=$Fps,scale=${Width}:${Height}:flags=lanczos,split[a][b];" +
              "[a]palettegen=stats_mode=full[p];[b][p]paletteuse=dither=bayer:bayer_scale=3"

    # -min_delay 0 は入力オプション (-i より前) なので位置に注意。
    # 既定の min_delay=2 だと、1cs の遅延が default_delay=10cs に置き換えられてしまう。
    # 素材は 1〜4cs なので、既定のままでは尺が約2.8倍に伸び、
    # 変換後のアニメーションがゲーム中で大幅に遅くなる。
    # エンジン (GifLoader) は遅延をそのまま読むので、ここも literal に読ませて合わせる。
    & $FFmpeg -hide_banner -loglevel error -y -min_delay 0 -i $gif.FullName `
        -vf $filter -loop 0 $tmp
    if ($LASTEXITCODE -ne 0) {
        if (Test-Path $tmp) { Remove-Item $tmp -Force }
        throw "ffmpeg が失敗しました: $($gif.Name)"
    }

    Move-Item $tmp $gif.FullName -Force

    $after = Get-GifStats $gif.FullName
    $beforeTotal += $before.AtlasMB
    $afterTotal += $after.AtlasMB

    $results += [pscustomobject]@{
        Name     = $gif.Name
        Frames   = "$($before.Frames) -> $($after.Frames)"
        Size     = "$($before.Width)x$($before.Height) -> $($after.Width)x$($after.Height)"
        # 再生尺。変換前後でほぼ一致していないと、ゲーム中の再生速度が変わってしまう
        Seconds  = "{0:N2} -> {1:N2}" -f $before.Seconds, $after.Seconds
        BeforeMB = [Math]::Round($before.AtlasMB, 1)
        AfterMB  = [Math]::Round($after.AtlasMB, 1)
        FileMB   = "{0:N1} -> {1:N1}" -f $before.FileMB, $after.FileMB
    }

    $drift = [Math]::Abs($after.Seconds - $before.Seconds)
    if ($before.Seconds -gt 0 -and ($drift / $before.Seconds) -gt 0.15) {
        Write-Warning ("$($gif.Name): 再生尺が {0:N2}s -> {1:N2}s と 15% 以上ずれています。" -f $before.Seconds, $after.Seconds)
    }
}

$results | Format-Table -AutoSize

"VRAM (GIFアトラス合計): {0:N1} MB -> {1:N1} MB  ({2:N1}x 削減, {3:N1} MB 削減)" -f `
    $beforeTotal, $afterTotal, ($beforeTotal / [Math]::Max($afterTotal, 0.01)), ($beforeTotal - $afterTotal)
