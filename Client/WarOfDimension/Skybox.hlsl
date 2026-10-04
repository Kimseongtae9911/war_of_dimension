#include "Common.hlsl"

struct VS_SKYBOX_CUBEMAP_INPUT
{
    float3 position : POSITION;
};

struct VS_SKYBOX_CUBEMAP_OUTPUT
{
    float3 positionL : POSITION;
    float4 position : SV_POSITION;
};

VS_SKYBOX_CUBEMAP_OUTPUT VSSkyBox(VS_SKYBOX_CUBEMAP_INPUT input)
{
    VS_SKYBOX_CUBEMAP_OUTPUT output;

    output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
    output.positionL = input.position;

    return (output);
}

float4 PSSkyBox(VS_SKYBOX_CUBEMAP_OUTPUT input) : SV_TARGET
{
    float4 cColor = gtxtSkyCubeTexture.Sample(gssClamp, input.positionL);

	// Calculate the brightness of the current pixel
    float brightness = dot(cColor.rgb, float3(0.299, 0.587, 0.114));

	// Apply twinkling effect only to the bright parts of the Skybox
    float twinkleFactor = 0.0;
    if (brightness > 0.7)  // Adjust the threshold to define the bright parts
    {
        twinkleFactor = Twinkle(input.positionL, 1.0f, 0.2f, true);
    }

    if (brightness < 0.3)
        cColor.rgb *= 0.5f;


	// Secondary Skybox texture for the Milky Way effect
    float3 newUV = RotateTextureCoordinates(input.positionL.xyz, gCurrentTime * 0.1f);
    float3 newUV2 = RotateTextureCoordinates(input.positionL.xyz, gCurrentTime * 0.9f);

    float4 cSecondaryColor = gtxtSkyCubeTexture.Sample(gssClamp, newUV);

    float4 cSecondaryColor2 = gtxtSkyCubeTexture.Sample(gssClamp, newUV2);

	// Calculate the brightness of the rotated secondary color
    float rotatedBrightness = dot(cSecondaryColor.rgb, float3(0.299, 0.587, 0.114));
    float rotatedBrightness2 = dot(cSecondaryColor2.rgb, float3(0.299, 0.587, 0.114));

	// Blend the secondary texture with the original texture only if the brightness is above 0.9
    float blendFactor = 0.f;
    if (0.7 < rotatedBrightness)
        blendFactor = 0.5f;
    cColor.rgb = lerp(cColor.rgb, cSecondaryColor.rgb, blendFactor);

    blendFactor = 0.f;
    if (0.7 < rotatedBrightness2)
        blendFactor = 0.5f;
    cColor.rgb = lerp(cColor.rgb, cSecondaryColor2.rgb, blendFactor);

	// Apply the twinkling effect to the bright parts
    cColor.rgb += twinkleFactor;

    return (cColor);
}