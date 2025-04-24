struct PSInput
{
    float4 position : SV_POSITION;
};

cbuffer CameraBuffer : register(b0)
{
    float4x4 projViewMatrix; // Camera's View-Projection Matrix
};

PSInput VSMain(float4 position : POSITION)
{
    PSInput result;
    result.position = mul(float4(position.xyz, 1.0), projViewMatrix);

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(1, 1, 0, 1);
}
