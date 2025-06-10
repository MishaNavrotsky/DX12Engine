struct PSInput
{
    float4 position : SV_POSITION;
};



struct VertexShaderInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoords : TEXCOORD;
    float4 tangent : TANGENT;
};

cbuffer camera : register(b0)
{
    float4x4 viewMatrix;
    float4x4 viewReverseProjMatrix;

    float4 position;
};

PSInput VSMain(VertexShaderInput input, uint vertexID : SV_VertexID)
{
    PSInput result;
    result.position = mul(float4(input.position, 1.0), viewReverseProjMatrix);
    
    return result;
}




float4 PSMain(PSInput input) : SV_Target
{
    return float4(1.0, 1.0, 1.0, 1.0);
}
