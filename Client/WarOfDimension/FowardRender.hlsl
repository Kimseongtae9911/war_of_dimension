#include "Standard.hlsl"

float4 PSBlend(VS_STANDARD_OUTPUT input) : SV_TARGET
{
    float4 cAlbedoColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    if (gnTexturesMask & MATERIAL_ALBEDO_MAP)
        cAlbedoColor = gtxtAlbedoTexture.Sample(gssWrap, input.uv);
    float4 cSpecularColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    if (gnTexturesMask & MATERIAL_SPECULAR_MAP)
        cSpecularColor = gtxtSpecularTexture.Sample(gssWrap, input.uv);
    float4 cNormalColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    if (gnTexturesMask & MATERIAL_NORMAL_MAP)
        cNormalColor = gtxtNormalTexture.Sample(gssWrap, input.uv);
    float4 cMetallicColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    if (gnTexturesMask & MATERIAL_METALLIC_MAP)
        cMetallicColor = gtxtMetallicTexture.Sample(gssWrap, input.uv);
    float4 cEmissionColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    if (gnTexturesMask & MATERIAL_EMISSION_MAP)
        cEmissionColor = gtxtEmissionTexture.Sample(gssWrap, input.uv);

    float3 normalW;
    float4 cColor = cAlbedoColor + cSpecularColor + cMetallicColor + cEmissionColor;
	//float4 cColor = gtxtDissolveTexture.Sample(gssWrap, input.uv);
	//float4 cColor = float4(0,0,0,0);
    if (gnTexturesMask & MATERIAL_NORMAL_MAP)
    {
        float3x3 TBN = float3x3(normalize(input.tangentW), normalize(input.bitangentW), normalize(input.normalW));
        float3 vNormal = normalize(cNormalColor.rgb * 2.0f - 1.0f); //[0, 1] ¡æ [-1, 1]
        normalW = normalize(mul(vNormal, TBN));
    }
    else
    {
        normalW = normalize(input.normalW);
        //discard;
    }

    float4 ShadowPosH = mul(float4(input.positionW, 1.0f), gmtxShadowTransform);
    float ShadowFactor = CalcShadowFactor(ShadowPosH, 0.0f);
    if (ShadowPosH.x > 1 || ShadowPosH.y > 1 || ShadowPosH.z > 1 || ShadowPosH.x < 0 || ShadowPosH.y < 0 || ShadowPosH.z < 0)
    {
        ShadowFactor = 1.0f;
    }
    ShadowFactor += 0.5f;
    ShadowFactor = saturate(ShadowFactor);
    float4 uvs[MAX_LIGHTS];
    float4 cIllumination = Lighting(input.positionW, normalW);
    if (cColor.a == 0)
        discard;
    cColor = (lerp(cColor, cIllumination, 0.5f)) * ShadowFactor;
    switch (gDrawOption)
    {
        case 1:
            cColor.rgb = DurandToneMapping(cColor.rgb); // Durand tone mapping
            break;
        case 2:
            cColor.rgb = Uncharted2Tonemap(cColor.rgb); // Uncharted2 tone mapping
            break;
        case 3:
            cColor.rgb = HableMcCannToneMapping(cColor.rgb, 1.0, float3(1.0, 1.0, 0.9)); // HableMcCannToneMapping
		//cColor.rgb = HableMcCannToneMappingAutoExposure(cColor.rgb, float3(1.0f, 1.0f, 1.0f), sceneExposure);	// HableMcCannToneMapping with auto exposure

            break;
        case 4:
	{	
                cColor.rgb = ACESFitted(cColor.rgb);
            }
            break;
        default:
            break;
    }
	


    return cColor;
}