#define RS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
              "DescriptorTable(CBV(b0))," \
              "DescriptorTable(SRV(t0, numDescriptors = 5))"

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texcoords : TEXCOORD;
    float4 tangent : TANGENT;
};

cbuffer CameraBuffer : register(b0)
{
    float4x4 projViewMatrix; // Camera's View-Projection Matrix
};


struct VertexShaderInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoords : TEXCOORD;
    float4 tangent : TANGENT;
};

[RootSignature(RS)]
PSInput VSMain(VertexShaderInput input)
{
    PSInput result;
    result.position = mul(float4(input.position, 1.0), projViewMatrix);
    result.normal = input.normal;
    result.tangent = input.tangent;
    result.texcoords = input.texcoords;

    return result;
}

[RootSignature(RS)]
float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(input.normal.xyz, 1.0);
}
