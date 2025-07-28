struct Well
{
    float4x4 skeletonSpaceMatrix;
    float4x4 skeletonSpaceInverseTransposeMatrix;
};

struct Vertex
{
    float32_t4 position;
    float32_t2 texcoord;
    float32_t3 normal;
};

struct VertexInfluence
{
    float32_t4 weight;
    int32_t4 index;
};

struct SkinningInformation
{
    uint32_t numVertices;
};

StructuredBuffer<Well> gMatrixPalette : register(t0);

StructuredBuffer<Vertex> gInputVertices : register(t1);

StructuredBuffer<VertexInfluence> gInfluence : register(t2);



[numthreads(1024, 1, 1)]
void main( uint32_t3 DTid : SV_DispatchThreadID )
{
    
}