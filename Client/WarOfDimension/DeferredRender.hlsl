#include "Standard.hlsl"

float4 DeferredDirectionalLight(int nIndex, float3 vNormal, float3 vToCamera, float4 vSpecular, float4 vDiffuse, float4 vAmbient)
{
    float3 vToLight = -gLights[nIndex].m_vDirection;
    float fDiffuseFactor = dot(vToLight, vNormal);
    float fSpecularFactor = 0.0f;
    if (fDiffuseFactor > 0.0f)
    {
        if (vSpecular.a != 0.0f)
        {
#ifdef _WITH_REFLECT
            float3 vReflect = reflect(-vToLight, vNormal);
            fSpecularFactor = pow(max(dot(vReflect, vToCamera), 0.0f), vSpecular.a);
#else
#ifdef _WITH_LOCAL_VIEWER_HIGHLIGHTING
			float3 vHalf = normalize(vToCamera + vToLight);
#else
			float3 vHalf = float3(0.0f, 1.0f, 0.0f);
#endif
			fSpecularFactor = pow(max(dot(vHalf, vNormal), 0.0f), vSpecular.a);
#endif
        }
    }

    return ((gLights[nIndex].m_cAmbient * vAmbient) + (gLights[nIndex].m_cDiffuse * fDiffuseFactor * vDiffuse) + (gLights[nIndex].m_cSpecular * fSpecularFactor * vSpecular));
}

float4 DeferredPointLight(int nIndex, float3 vPosition, float3 vNormal, float3 vToCamera, float4 vSpecular, float4 vDiffuse, float4 vAmbient)
{
    float3 vToLight = gLights[nIndex].m_vPosition - vPosition;
    float fDistance = length(vToLight);
    if (fDistance <= gLights[nIndex].m_fRange)
		//if (true)
    {
        float fSpecularFactor = 0.0f;
        vToLight /= fDistance;
        float fDiffuseFactor = dot(vToLight, vNormal);
        if (fDiffuseFactor > 0.0f)
        {
            if (vSpecular.a != 0.0f)
            {
#ifdef _WITH_REFLECT
                float3 vReflect = reflect(-vToLight, vNormal);
                fSpecularFactor = pow(max(dot(vReflect, vToCamera), 0.0f), vSpecular.a);
#else
#ifdef _WITH_LOCAL_VIEWER_HIGHLIGHTING
				float3 vHalf = normalize(vToCamera + vToLight);
#else
				float3 vHalf = float3(0.0f, 1.0f, 0.0f);
#endif
				fSpecularFactor = pow(max(dot(vHalf, vNormal), 0.0f), vSpecular.a);
#endif
            }
        }
        float fAttenuationFactor = 1.0f / dot(gLights[nIndex].m_vAttenuation, float3(1.0f, fDistance, fDistance * fDistance));
		//float fAttenuationFactor = 1.0f;

        return (((gLights[nIndex].m_cAmbient * vAmbient) + (gLights[nIndex].m_cDiffuse * fDiffuseFactor * vDiffuse) + (gLights[nIndex].m_cSpecular * fSpecularFactor * vSpecular)) * fAttenuationFactor);
    }
    return (float4(0.0f, 0.0f, 0.0f, 0.0f));
}

float4 DeferredSpotLight(int nIndex, float3 vPosition, float3 vNormal, float3 vToCamera, float4 vSpecular, float4 vDiffuse, float4 vAmbient)
{
    float3 vToLight = gLights[nIndex].m_vPosition - vPosition;
    float fDistance = length(vToLight);
    if (fDistance <= gLights[nIndex].m_fRange)
    {
        float fSpecularFactor = 0.0f;
        vToLight /= fDistance;
        float fDiffuseFactor = dot(vToLight, vNormal);
        if (fDiffuseFactor > 0.0f)
        {
            if (vSpecular.a != 0.0f)
            {
#ifdef _WITH_REFLECT
                float3 vReflect = reflect(-vToLight, vNormal);
                fSpecularFactor = pow(max(dot(vReflect, vToCamera), 0.0f), vSpecular.a);
#else
#ifdef _WITH_LOCAL_VIEWER_HIGHLIGHTING
				float3 vHalf = normalize(vToCamera + vToLight);
#else
				float3 vHalf = float3(0.0f, 1.0f, 0.0f);
#endif
				fSpecularFactor = pow(max(dot(vHalf, vNormal), 0.0f), vSpecular.a);
#endif
            }
        }
#ifdef _WITH_THETA_PHI_CONES
        float fAlpha = max(dot(-vToLight, gLights[nIndex].m_vDirection), 0.0f);
        float fSpotFactor = pow(max(((fAlpha - gLights[nIndex].m_fPhi) / (gLights[nIndex].m_fTheta - gLights[nIndex].m_fPhi)), 0.0f), gLights[nIndex].m_fFalloff);
#else
		float fSpotFactor = pow(max(dot(-vToLight, gLights[i].m_vDirection), 0.0f), gLights[i].m_fFalloff);
#endif
        float fAttenuationFactor = 1.0f / dot(gLights[nIndex].m_vAttenuation, float3(1.0f, fDistance, fDistance * fDistance));

        return (((gLights[nIndex].m_cAmbient * vAmbient) + (gLights[nIndex].m_cDiffuse * fDiffuseFactor * vDiffuse) + (gLights[nIndex].m_cSpecular * fSpecularFactor * vSpecular)) * fAttenuationFactor * fSpotFactor);
    }
    return (float4(0.0f, 0.0f, 0.0f, 0.0f));
}

float4 DeferredLighting(float3 vPosition, float3 vNormal, float4 vSpecular, float4 vDiffuse, float4 vAmbient)
{
    float3 vCameraPosition = float3(gvCameraPosition.x, gvCameraPosition.y, gvCameraPosition.z);
	//float3 vCameraPosition = float3(0.0f, 0.0f, 0.0f);
    float3 vToCamera = normalize(vCameraPosition - vPosition);

    float4 cColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	[unroll(MAX_LIGHTS)]
    for (int i = 0; i < gnLights; i++)
    {
        if (gLights[i].m_bEnable)
        {
            if (gLights[i].m_nType == DIRECTIONAL_LIGHT)
            {
                cColor += DeferredDirectionalLight(i, vNormal, vToCamera, vSpecular, vDiffuse, vAmbient);
            }
            else if (gLights[i].m_nType == POINT_LIGHT)
            {
                cColor += DeferredPointLight(i, vPosition, vNormal, vToCamera, vSpecular, vDiffuse, vAmbient);
            }
            else if (gLights[i].m_nType == SPOT_LIGHT)
            {
                cColor += DeferredSpotLight(i, vPosition, vNormal, vToCamera, vSpecular, vDiffuse, vAmbient);
            }
        }
    }
    cColor += (gcGlobalAmbientLight * vAmbient);
    cColor.a = vDiffuse.a;
	//cColor.a = 1.0f;

    return (cColor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

struct PS_MULTIPLE_RENDER_TARGETS_OUTPUT
{
    float4 f4Scene : SV_TARGET0;

    float4 f4Texture : SV_TARGET1;
    float4 f4Normal : SV_TARGET2;
    float4 f4Position : SV_TARGET3;
    float4 f4ObjectInfo : SV_TARGET4;
    float4 f4Ambient : SV_TARGET5;
    float4 f4Diffuse : SV_TARGET6;
};

PS_MULTIPLE_RENDER_TARGETS_OUTPUT PSTexturedAnimationObjMultipleRTs(VS_STANDARD_OUTPUT input)
{
    PS_MULTIPLE_RENDER_TARGETS_OUTPUT output;

	//output.f4Texture = gtxtAlbedoTexture.Sample(gssWrap, input.uv);
	
    output.f4Texture = float4(0.0f, 0.0f, 0.0f, 1.0f);
    if (gnTexturesMask & MATERIAL_ALBEDO_MAP)
        output.f4Texture = gtxtAlbedoTexture.Sample(gssWrap, input.uv);
    if (gnObjectID == 3)
        output.f4Texture.xyz = RimLighting(output.f4Texture.xyz, input.positionW, input.normalW, float3(1.0f, 0.0f, 0.0f)); 
    float4 cColor = gtxtDissolveTexture.Sample(gssWrap, input.uv);
    float dissolve = smoothstep(0, 1.0, cColor.r);
		
    output.f4Texture.a = 1;

    if (gnObjectState > 0)
    {
        float4 appearColor = float4(1.0f, 0.0f, 0.0f, 1.0f); // 색상을 원하는 값으로 설정
        float appearFactor = smoothstep(0.0, 1.0, gnObjectState); // 원하는 등장 시간 범위로 설정
        
		
        output.f4Texture.a -= gnObjectState * dissolve;
        if (output.f4Texture.a < 0.1)
            output.f4Texture.rgb += appearColor.rgb * appearFactor;
        if (output.f4Texture.a < 0.0)
            clip(output.f4Texture.a);
    }
	
    input.normalW = normalize(input.normalW);
    output.f4Normal = float4(input.normalW, 0);

    output.f4Position = float4(input.positionW, 1.0);

    float depth = 1.0f - input.position.z;
    output.f4ObjectInfo = float4(gnObjectID / 100.0, depth, 0.0f, 1.0f);
    output.f4Ambient = float4(0.0f, 0.0f, 0.0f, 1.0f);

    output.f4Diffuse = gMaterial.m_cDiffuse;
	
    output.f4Scene = output.f4Texture + gMaterial.m_cEmissive;

    return (output);
}

PS_MULTIPLE_RENDER_TARGETS_OUTPUT PSTexturedStandardMultipleRTs(VS_STANDARD_OUTPUT input)
{
    PS_MULTIPLE_RENDER_TARGETS_OUTPUT output;

    output.f4Texture = float4(0.0f, 0.0f, 0.0f, 1.0f);
    if (gnTexturesMask & MATERIAL_ALBEDO_MAP)
        output.f4Texture = gtxtAlbedoTexture.Sample(gssWrap, input.uv);

    input.normalW = normalize(input.normalW);

    output.f4Normal = float4(input.normalW, 0);

    output.f4Position = float4(input.positionW, 0);

    float depth = 1.0f - input.position.z;
    output.f4ObjectInfo = float4(gnObjectID / 100.0, depth, 0.0f, 1.0f);
    output.f4Ambient = float4(0.0f, 0.0f, 0.0f, 1.0f);

    output.f4Diffuse = gMaterial.m_cDiffuse;
	
    output.f4Scene = output.f4Texture + gMaterial.m_cEmissive;

    return (output);
}

struct VS_SCREEN_RECT_TEXTURED_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 viewSpaceDir : TEXCOORD1;
};

VS_SCREEN_RECT_TEXTURED_OUTPUT VSScreenRectSamplingTextured(uint nVertexID : SV_VertexID)
{
    VS_SCREEN_RECT_TEXTURED_OUTPUT output = (VS_SCREEN_RECT_TEXTURED_OUTPUT) 0;

    if (nVertexID == 0)
    {
        output.position = float4(-1.0f, +1.0f, 0.0f, 1.0f);
        output.uv = float2(0.0f, 0.0f);
    }
    else if (nVertexID == 1)
    {
        output.position = float4(+1.0f, +1.0f, 0.0f, 1.0f);
        output.uv = float2(1.0f, 0.0f);
    }
    else if (nVertexID == 2)
    {
        output.position = float4(+1.0f, -1.0f, 0.0f, 1.0f);
        output.uv = float2(1.0f, 1.0f);
    }
    else if (nVertexID == 3)
    {
        output.position = float4(-1.0f, +1.0f, 0.0f, 1.0f);
        output.uv = float2(0.0f, 0.0f);
    }
    else if (nVertexID == 4)
    {
        output.position = float4(+1.0f, -1.0f, 0.0f, 1.0f);
        output.uv = float2(1.0f, 1.0f);
    }
    else if (nVertexID == 5)
    {
        output.position = float4(-1.0f, -1.0f, 0.0f, 1.0f);
        output.uv = float2(0.0f, 1.0f);
    }

    output.viewSpaceDir = mul(output.position, gmtxProjection).xyz;

    return (output);
}

float4 PSScreenRectSamplingTextured(VS_SCREEN_RECT_TEXTURED_OUTPUT input) : SV_Target
{
    float2 textureSize = float2(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
    float2 texelCoord = input.uv * textureSize;
    uint3 texCoord = uint3(texelCoord, 0);

    float4 Texture = gtxtInputTextures[0].Load(texCoord);
    float3 Normal = gtxtInputTextures[1].Load(texCoord).rgb;
    float4 Position = gtxtInputTextures[2].Load(texCoord);
    float4 Specular = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 Ambient = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 Diffuse = gtxtInputTextures[5].Load(texCoord);

    float4 objectInfo = gtxtInputTextures[3].Load(texCoord);
    float oid = objectInfo.r * 100.0;
    float depth = objectInfo.g;
    
	//Position.x = Position.x * (POSITION_MAX - POSITION_MIN) + POSITION_MIN;
	//Position.y = Position.y * (POSITION_MAX - POSITION_MIN) + POSITION_MIN;
	//Position.z = Position.z * (POSITION_MAX - POSITION_MIN) + POSITION_MIN; //문제점 aliasing
    float3 pos = Position.xyz;
		
    float4 ShadowPosH = mul(float4(gtxtInputTextures[2][int2(input.position.xy)].xyz, 1.0f), gmtxShadowTransform);
    float biasValue = 0.000f;
    float ShadowFactor = CalcShadowFactor(ShadowPosH, biasValue);
	
    if (ShadowPosH.x > 1 || ShadowPosH.y > 1 || ShadowPosH.z > 1 || ShadowPosH.x < 0 || ShadowPosH.y < 0 || ShadowPosH.z < 0)
    {
        ShadowFactor = 1.0f;
    }
    ShadowFactor += 0.5f;

    ShadowFactor = saturate(ShadowFactor);
    
    float4 Illumination = DeferredLighting(pos, Normal, Specular, Diffuse, Ambient);

    float4 cColor = Texture;
    if (cColor.b >= 0.3f && cColor.r <= 0.1f && cColor.g <= 0.1f)
        discard;
    cColor.a = 1.0f;
	
	//cColor = MotionBlur(input.uv, float2(0.1f, 0.0f), 0.1f, 0.0016f);
    bool isTexture;
    if (max(cColor.r, max(cColor.g, cColor.b)) != 0.f)
        isTexture = true;
    else
        isTexture = false;

    if (isTexture)
        cColor = (lerp(cColor, Illumination, 0.5f)) * ShadowFactor;
    else
        cColor = (lerp(cColor, Illumination, 1.f)) * ShadowFactor;

    float3 histogram = float3(0.0f, 0.0f, 0.0f);
    const int numBins = 256;
    for (int i = 0; i < numBins; i++)
    {
        float4 binColor = gtxtInputTextures[0].Load(uint3(i, 0, 0));
        histogram.r += binColor.r;
        histogram.g += binColor.g;
        histogram.b += binColor.b;
    }

	// Normalize the histogram values
    histogram /= (numBins * 3.0f); // Divide by the total number of bins times the number of color channels (RGB)

    float sceneExposure = log2(0.18f / dot(histogram, float3(0.299f, 0.587f, 0.114f)));

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
	

	// Perform ColorGrading on the finished Scene texture.
    cColor = ColorGrading(cColor, exposure, saturation, contrast, vibrance);

	//cColor = ExponentialHeightFog(cColor, pos, normalize(pos - gvCameraPosition));

	// Sobel edge detection
    float4 cSobel = Sobel(gtxtInputTextures[0], input.uv, gssWrap);

	// Scale the Sobel output to reduce outline thickness
    if (oid <= 1.0f)
        cSobel.r *= 1.0f;
    else if (oid <= 4.0f)
        cSobel.r *= 1.0f;
    else if (oid <= 8.0f)
        cSobel.r *= 0.3f;
    else
        cSobel.r *= 0.0f;

    cSobel.r *= oid <= 4.0f ? depth : 1.0f;
    cSobel.r = gOutline ? cSobel.r : 0.0f;
	// Combine the Sobel edge detection with the original image
    cColor.rgb *= (1.0f - cSobel.r);

	// Modified Sobel edge dectction color
    if (oid <= 1.0f)
        cColor.rgb += cSobel.r * float3(0.0f, 0.0f, 0.0f);
    else if (oid <= 2.0f)
        cColor.rgb += cSobel.r * float3(5.0f, 0.0f, 0.0f);
    else if (oid <= 3.0f)
        cColor.rgb += cSobel.r * float3(0.0f, 0.0f, 0.0f);
    else if (oid <= 4.0f)
        cColor.rgb += cSobel.r * float3(0.0f, 0.0f, 0.0f);
    else if (oid <= 5.1f)
        cColor.rgb += cSobel.r * float3(0.502f, 0.9843f, 0.3137f);
    else if (oid <= 6.1f)
        cColor.rgb += cSobel.r * float3(0.9843f, 0.8157f, 0.3137f);
    else if (oid <= 7.1f)
        cColor.rgb += cSobel.r * float3(1.0f, 0.4314f, 0.4314f);
    else if (oid <= 8.1f)
        cColor.rgb += cSobel.r * float3(0.9765f, 0.455f, 0.7686f);
    else
        cColor.rgb += cSobel.r * float3(0.0f, 0.0f, 0.0f);


    return cColor;
}