#define RS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
              "DescriptorTable( CBV(b0))"

struct PSInput
{
    float4 position : SV_POSITION;
};

cbuffer CameraBuffer : register(b0)
{
    float4x4 projViewMatrix; // Camera's View-Projection Matrix
};

[RootSignature(RS)]
PSInput VSMain(float4 position : POSITION)
{
    PSInput result;
    result.position = mul(float4(position.xyz, 1.0), projViewMatrix);

    return result;
}

[RootSignature(RS)]
float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(1, 1, 0, 1);
}
