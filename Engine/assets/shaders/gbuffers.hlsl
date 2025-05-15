struct PSInput
{
    float4 position : SV_POSITION;
    float4 worldPosition : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
    float3 worldTangent : TEXCOORD2;
    float4 tangent : TANGENT;
    float2 texcoords : TEXCOORD3;
    
    float3 ndcPosition : TEXCOORD4;
    float3 ndcPrevPosition : TEXCOORD5;
};

struct CPUMaterialCBVData
{
    float4 emissiveFactor; // RGBA emissive intensity
    float4 baseColorFactor; // RGBA base color

    float4 normalScaleOcclusionStrengthMRFactors; // Normal, Occlusion, Metallic, Roughness scaling

    uint4 diffuseEmissiveNormalOcclusionTexSlots; // Bindless texture indices
    uint4 MrTexSlots; // Metallic-Roughness texture slot

    uint4 diffuseEmissiveNormalOcclusionSamSlots; // Bindless sampler indices
    uint4 MrSamSlots; // Metallic-Roughness sampler slot
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

PSInput VSMain(VertexShaderInput input)
{
    PSInput result;
    result.position = mul(float4(input.position, 1.0), MVrP);
    result.texcoords = input.texcoords;
    result.worldPosition = mul(float4(input.position, 1.0), modelMatrix);
    result.worldTangent = mul(input.tangent.xyz, (float3x3) modelMatrix);
    result.worldNormal = mul(input.normal, (float3x3) modelMatrix);
    result.tangent = input.tangent;
    // Convert to NDC
    float4 ndcPosition = result.position / result.position.w;
    float4 ndcPrevPosition = mul(float4(input.position, 1.0), prevModelMatrix);
    ndcPrevPosition = mul(ndcPrevPosition, prevViewProjectionReverseDepthMatrix);
    ndcPrevPosition = ndcPrevPosition / ndcPrevPosition.w;

    result.ndcPosition = ndcPosition.xyz;
    result.ndcPrevPosition = ndcPrevPosition.xyz;
    
    return result;
}

struct PSOutput
{
    float4 albedo : SV_Target0;
    float4 worldNormals : SV_Target1;
    float4 worldPosition : SV_Target2;
    float4 roughnessMetalAo : SV_Target3;
    float4 emissive : SV_Target4;
    float4 screenSpaceMotion : SV_Target5;
    uint materialID : SV_Target6;
};

ConstantBuffer<CPUMaterialCBVData> g_cbvs[] : register(b2, space0);
Texture2D<float4> g_textures[] : register(t0, space0);
SamplerState g_samplers[] : register(s0, space0);


PSOutput PSMain(PSInput input)
{
    PSOutput output;

    ConstantBuffer<CPUMaterialCBVData> cbvMat = g_cbvs[cbvDataBindlessHeapSlot.x];
    Texture2D<float4> diffuseTex = g_textures[cbvMat.diffuseEmissiveNormalOcclusionTexSlots.x];
    Texture2D<float4> emissiveTex = g_textures[cbvMat.diffuseEmissiveNormalOcclusionTexSlots.y];
    Texture2D<float4> normalTex = g_textures[cbvMat.diffuseEmissiveNormalOcclusionTexSlots.z];
    Texture2D<float4> occlusionTex = g_textures[cbvMat.diffuseEmissiveNormalOcclusionTexSlots.w];
    Texture2D<float4> mrTex = g_textures[cbvMat.MrTexSlots.x];
    
    SamplerState diffuseSam = g_samplers[cbvMat.diffuseEmissiveNormalOcclusionSamSlots.x];
    SamplerState emissiveSam = g_samplers[cbvMat.diffuseEmissiveNormalOcclusionSamSlots.y];
    SamplerState normalSam = g_samplers[cbvMat.diffuseEmissiveNormalOcclusionSamSlots.z];
    SamplerState occlusionSam = g_samplers[cbvMat.diffuseEmissiveNormalOcclusionSamSlots.w];
    SamplerState mrSam = g_samplers[cbvMat.MrSamSlots.x];
    
    float normalScale = cbvMat.normalScaleOcclusionStrengthMRFactors.x;
    float occlusionStrength = cbvMat.normalScaleOcclusionStrengthMRFactors.y;
    float roughnessFactor = cbvMat.normalScaleOcclusionStrengthMRFactors.w;
    float metallicFactor = cbvMat.normalScaleOcclusionStrengthMRFactors.z;
    
    float4 diffuse = diffuseTex.Sample(diffuseSam, input.texcoords) * cbvMat.baseColorFactor;
    float4 emissive = emissiveTex.Sample(emissiveSam, input.texcoords) * cbvMat.emissiveFactor;
    
    float3 normal = normalTex.Sample(normalSam, input.texcoords).rgb;
    normal = normal * 2.0f - 1.0f;
    normal.xy *= normalScale;
    float xyDot = dot(normal.xy, normal.xy);
    normal.z = sqrt(max(0.0f, 1.0f - xyDot));
    float3 worldBitangent = normalize(cross(input.worldNormal, input.worldTangent) * input.tangent.w);
    float3x3 TBN = float3x3(input.worldTangent, worldBitangent, input.worldNormal);
    normal = normalize(mul(normal, TBN));
    
    float occlusion = lerp(1.0f, occlusionTex.Sample(occlusionSam, input.texcoords).r, occlusionStrength);
    float2 rm = mrTex.Sample(mrSam, input.texcoords).gb * float2(roughnessFactor, metallicFactor);
    
    output.albedo = diffuse;
    output.worldNormals = float4(normal, 0.0);
    output.worldPosition = float4(input.worldPosition.xyz, 0.0);
    output.roughnessMetalAo = float4(rm, occlusion, 0.0);
    output.emissive = float4(emissive.x, emissive.y, emissive.z, 0.0);
    output.materialID = cbvDataBindlessHeapSlot.x;
    
    float2 motionNDC = input.ndcPosition.xy - input.ndcPrevPosition.xy;
    float motionZ = input.ndcPosition.z - input.ndcPrevPosition.z;

    float2 motionXY = 0.5 * motionNDC * screenDimensions.xy;

    output.screenSpaceMotion = float4(motionXY, motionZ, 0.0);
        
    return output;
}
