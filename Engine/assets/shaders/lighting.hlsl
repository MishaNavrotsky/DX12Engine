#define RS \
    "CBV(b0, flags=DATA_STATIC), " \
    "CBV(b1, flags=DATA_STATIC), " \
    "DescriptorTable(" \
    "SRV(t0)," \
    "SRV(t1)," \
    "SRV(t2)," \
    "SRV(t3)," \
    "SRV(t4)," \
    "SRV(t5)," \
    "SRV(t6)," \
    "UAV(u0)" \
    ")"


cbuffer camera : register(b0)
{
    float4x4 viewMatrix;
    float4x4 viewReverseProjMatrix;

    float4 position;
};
cbuffer globals : register(b1)
{
    uint transformsIndex, screenX, screenY, unsued;
};

Texture2D<float4> albedoBuffer : register(t0); // Albedo G-buffer
Texture2D<float4> worldNormalsBuffer : register(t1); // World normals G-buffer
Texture2D<float4> worldPositionBuffer : register(t2); // World position G-buffer
Texture2D<float4> roughnessMetalAoBuffer : register(t3); // Roughness + Metalness + AO G-buffer
Texture2D<float4> emissiveBuffer : register(t4); // Emissive G-buffer
Texture2D<float4> screenSpaceMotionBuffer : register(t5); // ScreenSpace Motion Vectors G-buffer
Texture2D<uint> materialIDBuffer : register(t6); // Material ID buffer
RWTexture2D<float4> outputLighting : register(u0); // Output lighting texture

[RootSignature(RS)]
[numthreads(16, 16, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 screenPos = dispatchThreadID.xy;
    uint2 screenSize = float2(screenX, screenY);
    
    float3 rmao = roughnessMetalAoBuffer.Load(int3(screenPos, 0)).xyz;

    float4 albedo = albedoBuffer.Load(int3(screenPos, 0));
    float3 normal = worldNormalsBuffer.Load(int3(screenPos, 0)).xyz;
    float3 position = worldPositionBuffer.Load(int3(screenPos, 0)).xyz;
    float roughness = rmao.x;
    float metalness = rmao.y;
    float ambientOcclusion = rmao.z;
    float3 emissive = emissiveBuffer.Load(int3(screenPos, 0)).rgb;
 
    
    outputLighting[dispatchThreadID.xy] = float4(normal, 1.);
}