#include "Particle.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
};

ConstantBuffer<Material> gMaterial : register(b0);

// グループ単位の設定（ParticleGroupParam）とフラグは Particle.hlsli にある（頂点シェーダーも読むため）

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

Texture2D<float4> gTexture : register(t0);
// 炎・煙の質感を付けるノイズ（既定は FireNoise.jpg）
Texture2D<float4> gNoiseTexture : register(t1);
// シーンのワールド座標（GBuffer の WorldPos）。何も無い所は (0,0,0)
Texture2D<float4> gSceneWorldPos : register(t2);
SamplerState gSampler : register(s0);
// ノイズ用。繰り返し用に作られていない写真でも継ぎ目が線にならないよう鏡映しで繰り返す
SamplerState gNoiseSampler : register(s1);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float2 uv = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform).xy;

    float4 noise = float4(1.0f, 1.0f, 1.0f, 1.0f);
    const bool useNoise = (flags & kFlagNoise) != 0;
    if (useNoise)
    {
        // 粒ごとに読み出し位置をずらして、同じ模様が並ばないようにする
        float2 seedOffset = input.misc.x * float2(0.731f, 0.379f) * 4.0f;
        float2 noiseUV = (input.meshTexcoord - 0.5f) * noiseTiling + 0.5f + noiseScroll * time + seedOffset;
        noise = gNoiseTexture.Sample(gNoiseSampler, noiseUV);

        // 形の揺らぎ。模様と同じ所を読むと色と形が同じ向きにずれるだけなので、縮尺と位置を変えて読む
        float2 warp = gNoiseTexture.Sample(gNoiseSampler, noiseUV * 0.53f + float2(0.37f, 0.11f)).rg - 0.5f;
        uv += warp * 2.0f * noiseDistortion;
    }

    float4 textureColor = gTexture.Sample(gSampler, uv);
    float3 rgb = textureColor.rgb;
    float alpha = textureColor.a;

    if (useNoise)
    {
        float luminance = dot(noise.rgb, float3(0.299f, 0.587f, 0.114f));
        float3 noiseRgb = ((flags & kFlagGrayscale) != 0) ? luminance.xxx : noise.rgb;
        rgb = lerp(rgb, noiseRgb, noiseColorBlend);

        // 寿命が進むほど、暗い所から削れて千切れていく
        if (noiseErosion > 0.0f)
        {
            float threshold = input.misc.y * noiseErosion;
            alpha *= smoothstep(threshold - noiseErosionSoftness, threshold + noiseErosionSoftness, luminance);
        }
    }

    output.color = gMaterial.color * float4(rgb, alpha) * input.color;
    output.color.rgb *= emissiveIntensity;

    // 光源に照らされる（埃）。光の当たらない所ではほとんど見えず、松明や天井の光の中で浮かび上がる。
    // 色と透明度を別々の係数で変える（両方を強く変えると明るくなりすぎるため）
    if ((flags & kFlagLit) != 0)
    {
        float3 light = ambientDustColor + input.lighting;
        float luminance = dot(light, float3(0.2126f, 0.7152f, 0.0722f));
        output.color.rgb *= lerp(luminance.xxx, light, colorResponse);
        float lightLevel = saturate(luminance * lightResponse);
        output.color.a *= lerp(ambientVisibility, 1.0f, lightLevel);
    }

    // カメラからの距離で薄くする。目の前を横切る大きな粒と、遠くの粒を消す
    if ((flags & kFlagCameraFade) != 0)
    {
        float cameraDistance = distance(input.worldPos, cameraPosition);
        float nearFade = smoothstep(nearFadeStart, nearFadeEnd, cameraDistance);
        float farFade = 1.0f - smoothstep(farFadeStart, farFadeEnd, cameraDistance);
        output.color.a *= nearFade * farFade;
    }
    output.color.a *= alphaScale;

    // デバッグ表示（埃パーティクル仕様書 10.3）。光源に照らされるグループだけを、形の分だけ不透明で描く
    if (debugView > 1.5f)
    {
        if ((flags & kFlagLit) == 0)
            discard;
        float shape = saturate(alpha * 4.0f);
        if (shape <= 0.01f)
            discard;
        if (debugView < 2.5f)
            output.color = float4(ambientDustColor + input.lighting, shape); // 2: 受けた光
        else
            output.color = float4(saturate(output.color.a * 6.0f).xxx, shape); // 3: 透明度（6倍して見やすく）
        return output;
    }

    // ソフトパーティクル: 地面や壁に近い部分ほど薄くして、交わる所がくっきり切れないようにする
    if ((flags & kFlagSoft) != 0 && softDistance > 0.0f)
    {
        uint width;
        uint height;
        gSceneWorldPos.GetDimensions(width, height);
        int2 pixel = clamp(int2(input.position.xy), int2(0, 0), int2(width - 1, height - 1));
        float4 sceneSample = gSceneWorldPos.Load(int3(pixel, 0));
        float3 scenePos = sceneSample.xyz;
        // 何も描かれていない所（空）は w=0。そこでは薄くしない
        if (sceneSample.w > 0.5f)
        {
            float sceneDistance = distance(cameraPosition, scenePos);
            float particleDistance = distance(cameraPosition, input.worldPos);
            output.color.a *= saturate((sceneDistance - particleDistance) / softDistance);
        }
    }

    // output.colorのα値が0の時にPixelを棄却
    if (output.color.a == 0.0f)
    {
        discard;
    }

    return output;
}
