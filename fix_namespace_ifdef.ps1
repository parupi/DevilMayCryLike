
# #ifdef ブロック内に誤挿入された namespace Guchis { を正しい位置に移動する
param([string]$NS = "Guchis")

$NS_OPEN  = "namespace $NS {"

$targetFiles = @(
    "Engine\Debugger\GlobalVariables.cpp",
    "Engine\Debugger\ImGuiManager.cpp",
    "Engine\Graphics\Rendering\Particle\ParticleEditor.cpp",
    "Engine\Graphics\Rendering\RenderPath\RenderPipeline.cpp",
    "Engine\Graphics\Rendering\Shadow\CascadedShadowMap.cpp",
    "Engine\Graphics\Rendering\Sprite\Sprite.cpp",
    "Engine\Math\Matrix4x4.cpp",
    "Engine\Platform\WindowManager.cpp",
    "Engine\World3D\Camera\BaseCamera.cpp",
    "Engine\World3D\Light\DirectionalLight.cpp",
    "Engine\World3D\Light\PointLight.cpp",
    "Engine\World3D\Light\SpotLight.cpp",
    "Engine\World3D\Object\Object3d.cpp",
    "Engine\World3D\Object\Renderer\ModelRenderer.cpp",
    "Engine\World3D\Object\Renderer\PrimitiveRenderer.cpp",
    "Engine\World3D\WorldTransform.cpp"
)

$root = "C:\Users\kawah\source\repos\DevilMayCryLike"

foreach ($rel in $targetFiles) {
    $path = Join-Path $root $rel
    if (-not (Test-Path $path)) { Write-Host "NOT FOUND: $path"; continue }

    $raw   = [System.IO.File]::ReadAllText($path, [System.Text.Encoding]::UTF8)
    $text  = $raw -replace "`r`n", "`n" -replace "`r", "`n"
    $lines = $text -split "`n"

    # 1) `namespace Guchis {` が #ifdef より後にある行を探して削除
    $nsLineIdx = -1
    for ($i = 0; $i -lt $lines.Length; $i++) {
        if ($lines[$i].Trim() -eq $NS_OPEN) { $nsLineIdx = $i; break }
    }
    if ($nsLineIdx -lt 0) { Write-Host "NO NS_OPEN found: $path"; continue }

    # #ifdef に囲まれているか確認
    $insideIfdef = $false
    $depth = 0
    for ($i = 0; $i -lt $nsLineIdx; $i++) {
        $t = $lines[$i].Trim()
        if ($t -match '^#\s*if(def|ndef|)\b') { $depth++ }
        elseif ($t -match '^#\s*endif\b')       { $depth-- }
    }
    $insideIfdef = ($depth -gt 0)

    if (-not $insideIfdef) {
        Write-Host "OK (not in ifdef): $path"
        continue
    }

    # 2) #ifdef ブロックの先頭行インデックスを見つける（深さが現在の depth に達した行）
    $ifdefStartIdx = -1
    $d = 0
    for ($i = 0; $i -lt $nsLineIdx; $i++) {
        $t = $lines[$i].Trim()
        if ($t -match '^#\s*if(def|ndef|)\b') {
            $d++
            if ($d -eq $depth) { $ifdefStartIdx = $i }
        } elseif ($t -match '^#\s*endif\b') {
            $d--
        }
    }

    if ($ifdefStartIdx -lt 0) { Write-Host "COULD NOT FIND ifdef start: $path"; continue }

    # 3) namespace Guchis { を削除（その前後の空行も1つ削除）
    $newLines = [System.Collections.Generic.List[string]]$lines
    $newLines.RemoveAt($nsLineIdx)
    # 前後の余分な空行を1つ削除
    if ($nsLineIdx -lt $newLines.Count -and [string]::IsNullOrWhiteSpace($newLines[$nsLineIdx])) {
        $newLines.RemoveAt($nsLineIdx)
    }
    if ($nsLineIdx -gt 0 -and [string]::IsNullOrWhiteSpace($newLines[$nsLineIdx - 1])) {
        $newLines.RemoveAt($nsLineIdx - 1)
    }

    # 4) #ifdef の直前に namespace Guchis { を挿入
    # ifdefStartIdx は削除後にずれているので再計算
    $newIfdefIdx = -1
    for ($i = 0; $i -lt $newLines.Count; $i++) {
        $t = $newLines[$i].Trim()
        if ($t -match '^#\s*if(def|ndef|)\b') {
            $cnt = 0
            for ($j = 0; $j -lt $i; $j++) {
                $tj = $newLines[$j].Trim()
                if ($tj -match '^#\s*if(def|ndef|)\b') { $cnt++ }
                elseif ($tj -match '^#\s*endif\b')       { $cnt-- }
            }
            if ($cnt -eq $depth - 1) { $newIfdefIdx = $i; break }
        }
    }

    if ($newIfdefIdx -lt 0) {
        # 単純に元の位置の前に挿入
        $newIfdefIdx = $ifdefStartIdx
    }

    # 直前が空行でなければ空行を追加
    if ($newIfdefIdx -gt 0 -and -not [string]::IsNullOrWhiteSpace($newLines[$newIfdefIdx - 1])) {
        $newLines.Insert($newIfdefIdx, "")
        $newIfdefIdx++
    }
    $newLines.Insert($newIfdefIdx, $NS_OPEN)

    $newText = ($newLines -join "`n")
    [System.IO.File]::WriteAllText($path, $newText, [System.Text.Encoding]::UTF8)
    Write-Host "FIXED: $path"
}

Write-Host "`n=== 完了 ==="
