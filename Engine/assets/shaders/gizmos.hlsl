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

cbuffer cb0 : register(b0)
{
    float4x4 projectionMatrix;
    float4x4 projectionReverseDepthMatrix;
    float4x4 viewMatrix;
    float4x4 viewProjectionReverseDepthMatrix;
    
    float4x4 prevProjectionMatrix;
    float4x4 prevProjectionReverseDepthMatrix;
    float4x4 prevViewMatrix;
    float4x4 prevViewProjectionReverseDepthMatrix;
    float4 position;
    uint4 screenDimensions;
};

cbuffer cb1 : register(b1)
{
    float4x4 modelMatrix;
    float4x4 prevModelMatrix;
    float4x4 MVrP;

    
    uint4 cbvDataBindlessHeapSlot;
};

PSInput VSMain(VertexShaderInput input, uint vertexID : SV_VertexID)
{
    PSInput result;
    result.position = mul(float4(input.position, 1.0), viewProjectionReverseDepthMatrix);
    
    //float2 positions[3] =
    //{
    //    float2(-1.0, -1.0), // Bottom-left
    //    float2(3.0, -1.0), // Bottom-right (off-screen to ensure full coverage)
    //    float2(-1.0, 3.0) // Top-left (off-screen to ensure full coverage)
    //};
    
    //result.position = float4(1, 1, 1, 5); 
    //result.position.xy = positions[vertexID % 3];
    
    return result;
}




float4 PSMain(PSInput input) : SV_Target
{
    return float4(1.0, 1.0, 1.0, 1.0);
}
