//#define RS "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | SAMPLER_HEAP_DIRECTLY_INDEXED), " \
//            "CBV(b0, flags=DATA_STATIC), " \
//            "DescriptorTable(SRV(t0, numDescriptors = 10000)), " \
//            "DescriptorTable(CBV(b1, numDescriptors = 10000)), " \
//            "DescriptorTable(Sampler(s0, numDescriptors = 2048))"

#define RS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | SAMPLER_HEAP_DIRECTLY_INDEXED), \
     CBV(b0, flags=DATA_STATIC), \
     CBV(b1, flags=DATA_STATIC)"

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
    
    uint4 cbvDataBindlessHeapSlot;
};

[RootSignature(RS)]
PSInput VSMain(VertexShaderInput input)
{
    PSInput result;
    result.position = mul(float4(input.position, 1.0), modelMatrix);
    result.position = mul(result.position, viewProjectionReverseDepthMatrix);
    result.texcoords = input.texcoords;
    result.worldPosition = mul(float4(input.position, 1.0), modelMatrix);
    result.worldTangent = mul(input.tangent.xyz, (float3x3)modelMatrix);
    result.worldNormal = mul(input.normal, (float3x3)modelMatrix);
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

[RootSignature(RS)]
PSOutput PSMain(PSInput input)
{
    PSOutput output;

    ConstantBuffer<CPUMaterialCBVData> cbvMat = ResourceDescriptorHeap[cbvDataBindlessHeapSlot.x];
    Texture2D<float4> diffuseTex = ResourceDescriptorHeap[cbvMat.diffuseEmissiveNormalOcclusionTexSlots.x];
    Texture2D<float4> emissiveTex = ResourceDescriptorHeap[cbvMat.diffuseEmissiveNormalOcclusionTexSlots.y];
    Texture2D<float4> normalTex = ResourceDescriptorHeap[cbvMat.diffuseEmissiveNormalOcclusionTexSlots.z];
    Texture2D<float4> occlusionTex = ResourceDescriptorHeap[cbvMat.diffuseEmissiveNormalOcclusionTexSlots.w];
    Texture2D<float4> mrTex = ResourceDescriptorHeap[cbvMat.MrTexSlots.x];
    
    SamplerState diffuseSam = SamplerDescriptorHeap[cbvMat.diffuseEmissiveNormalOcclusionSamSlots.x];
    SamplerState emissiveSam = SamplerDescriptorHeap[cbvMat.diffuseEmissiveNormalOcclusionSamSlots.y];
    SamplerState normalSam = SamplerDescriptorHeap[cbvMat.diffuseEmissiveNormalOcclusionSamSlots.z];
    SamplerState occlusionSam = SamplerDescriptorHeap[cbvMat.diffuseEmissiveNormalOcclusionSamSlots.w];
    SamplerState mrSam = SamplerDescriptorHeap[cbvMat.MrSamSlots.x];
    
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
    output.screenSpaceMotion = float4(1, 1, 1, 1);
    output.materialID = cbvDataBindlessHeapSlot.x;
    
    float2 motionNDC = input.ndcPosition.xy - input.ndcPrevPosition.xy;
    float motionZ = input.ndcPosition.z - input.ndcPrevPosition.z;

    float2 motionXY = 0.5 * motionNDC * screenDimensions.xy;

    output.screenSpaceMotion = float4(motionXY, motionZ, 0.0);
    
    return output;
}
