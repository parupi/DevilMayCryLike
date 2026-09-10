cbuffer MarkerCB : register(b0)
{
    float4x4 viewProj;
};

struct VSInput
{
    float3 position : POSITION;   // ワールド座標（地面に寝かせた四角形の頂点）
    float2 local    : TEXCOORD0;  // 図形ローカル[m]。x=右 / y=前（敵の正面）
    float4 param    : TEXCOORD1;  // x=形状 y=進み具合(0-1) z=扇の半角[rad] w=不透明度
    float4 extent   : TEXCOORD2;  // x=横の広さ[m] y=奥行きの広さ[m] z=閃光 w=枠の太さ[m]
    float4 color    : COLOR;      // rgb=マーカーの色
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 local    : TEXCOORD0;
    float4 param    : TEXCOORD1;
    float4 extent   : TEXCOORD2;
    float4 color    : COLOR;
};

PSInput main(VSInput input)
{
    PSInput output;
    output.position = mul(float4(input.position, 1.0f), viewProj);
    output.local  = input.local;
    output.param  = input.param;
    output.extent = input.extent;
    output.color  = input.color;
    return output;
}
