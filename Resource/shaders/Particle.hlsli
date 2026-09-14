struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
    // 粒のワールド座標（ソフトパーティクルが地面との距離を測るのに使う）
    float3 worldPos : TEXCOORD1;
    // x = 粒ごとの乱数 / y = 寿命の進み具合
    float4 misc : TEXCOORD2;
    // スプライトシートで切り出す前のUV。ノイズはコマに関係なく板全体に貼る
    float2 meshTexcoord : TEXCOORD3;
};
