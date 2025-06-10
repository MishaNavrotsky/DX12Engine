struct VSInput
{
    float3 position : POSITION;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    
    output.position = float4(input.position, 1.0f);
    output.texCoord = (input.position.xy + 1.0f) * 0.5f;

    return output;
}

cbuffer globals : register(b0)
{
    uint transformsIndex, screenX, screenY, unsued;
};

Texture2D<float4> lightingTexture : register(t0); // Lighting texture (SRV at slot 1)
Texture2D<float4> gizmosTexture : register(t1); // Gizmos texture (SRV at slot 0)
Texture2D<float4> uiTexture : register(t2); // Gizmos texture (SRV at slot 0)


float4 PSMain(VSOutput input) : SV_Target
{
    float2 screenDimensions = float2(screenX, screenY);
    int3 texCoord = int3(input.texCoord.x * screenDimensions.x, (1.0 - input.texCoord.y) * screenDimensions.y, 0);
    float4 gizmos = gizmosTexture.Load(texCoord);
    float4 lighting = lightingTexture.Load(texCoord);
    float4 ui = uiTexture.Load(texCoord);
    
    float4 uiOverGizmos;
    uiOverGizmos.rgb = lerp(gizmos.rgb, ui.rgb, ui.a);
    uiOverGizmos.a = ui.a + gizmos.a * (1.0 - ui.a);

    float4 finalColor;
    finalColor.rgb = lerp(lighting.rgb, uiOverGizmos.rgb, uiOverGizmos.a);
    finalColor.a = uiOverGizmos.a + lighting.a * (1.0 - uiOverGizmos.a);

    return finalColor;
}