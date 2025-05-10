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
    float3 normal : NORMAL;
    float2 texcoords : TEXCOORD;
    float4 tangent : TANGENT;
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
    float4 position;
};

cbuffer cb1 : register(b1)
{
    float4x4 modelMatrix;
    uint4 cbvDataBindlessHeapSlot;
};

[RootSignature(RS)]
PSInput VSMain(VertexShaderInput input)
{
    PSInput result;
    result.position = mul(float4(input.position, 1.0), modelMatrix);
    result.position = mul(result.position, viewProjectionReverseDepthMatrix);
    result.normal = input.normal;
    result.tangent = input.tangent;
    result.texcoords = input.texcoords;

    return result;
}

[RootSignature(RS)]
float4 PSMain(PSInput input) : SV_TARGET
{
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
    
    float4 diffuse = diffuseTex.Sample(diffuseSam, input.texcoords) * cbvMat.baseColorFactor;
    float4 emissive = emissiveTex.Sample(emissiveSam, input.texcoords) * cbvMat.emissiveFactor;
    float4 normal = normalTex.Sample(normalSam, input.texcoords);
    float4 occlusion = occlusionTex.Sample(occlusionSam, input.texcoords);
    float4 mr = mrTex.Sample(mrSam, input.texcoords);
    
    return diffuse;
}
