struct MATERIAL
{
	float4					m_cAmbient;
	float4					m_cDiffuse;
	float4					m_cSpecular; //a = power
	float4					m_cEmissive;
};

cbuffer cbCameraInfo : register(b1)
{
	matrix					gmtxView : packoffset(c0);
	matrix					gmtxProjection : packoffset(c4);
	float3					gvCameraPosition : packoffset(c8);
};

cbuffer cbGameObjectInfo : register(b2)
{
	matrix					gmtxGameObject : packoffset(c0);
	MATERIAL				gMaterial : packoffset(c4);
	uint					gnTexturesMask : packoffset(c8);
};


#include "Light.hlsl"

#define POSITION_MAX 1000.f
#define POSITION_MIN -1000.f



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//#define _WITH_VERTEX_LIGHTING

#define MATERIAL_ALBEDO_MAP			0x01
#define MATERIAL_SPECULAR_MAP		0x02
#define MATERIAL_NORMAL_MAP			0x04
#define MATERIAL_METALLIC_MAP		0x08
#define MATERIAL_EMISSION_MAP		0x10
#define MATERIAL_DETAIL_ALBEDO_MAP	0x20
#define MATERIAL_DETAIL_NORMAL_MAP	0x40

Texture2D gtxtAlbedoTexture : register(t6);
Texture2D gtxtSpecularTexture : register(t7);
Texture2D gtxtNormalTexture : register(t8);
Texture2D gtxtMetallicTexture : register(t9);
Texture2D gtxtEmissionTexture : register(t10);
Texture2D gtxtDetailAlbedoTexture : register(t11);
Texture2D gtxtDetailNormalTexture : register(t12);

SamplerState gssWrap : register(s0);

struct VS_STANDARD_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 bitangent : BITANGENT;
};

struct VS_STANDARD_OUTPUT
{
	float4 position : SV_POSITION;
	float3 positionW : POSITION;
	float3 normalW : NORMAL;
	float3 tangentW : TANGENT;
	float3 bitangentW : BITANGENT;
	float2 uv : TEXCOORD;
};

VS_STANDARD_OUTPUT VSStandard(VS_STANDARD_INPUT input)
{
	VS_STANDARD_OUTPUT output;

	output.positionW = mul(float4(input.position, 1.0f), gmtxGameObject).xyz;
	output.normalW = mul(input.normal, (float3x3)gmtxGameObject);
	output.tangentW = mul(input.tangent, (float3x3)gmtxGameObject);
	output.bitangentW = mul(input.bitangent, (float3x3)gmtxGameObject);
	output.position = mul(mul(float4(output.positionW, 1.0f), gmtxView), gmtxProjection);
	output.uv = input.uv;

	return(output);
}

float4 PSStandard(VS_STANDARD_OUTPUT input) : SV_TARGET
{
	float4 cAlbedoColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
	if (gnTexturesMask & MATERIAL_ALBEDO_MAP) cAlbedoColor = gtxtAlbedoTexture.Sample(gssWrap, input.uv);
	float4 cSpecularColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
	if (gnTexturesMask & MATERIAL_SPECULAR_MAP) cSpecularColor = gtxtSpecularTexture.Sample(gssWrap, input.uv);
	float4 cNormalColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
	if (gnTexturesMask & MATERIAL_NORMAL_MAP) cNormalColor = gtxtNormalTexture.Sample(gssWrap, input.uv);
	float4 cMetallicColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
	if (gnTexturesMask & MATERIAL_METALLIC_MAP) cMetallicColor = gtxtMetallicTexture.Sample(gssWrap, input.uv);
	float4 cEmissionColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
	if (gnTexturesMask & MATERIAL_EMISSION_MAP) cEmissionColor = gtxtEmissionTexture.Sample(gssWrap, input.uv);

	float3 normalW;
	float4 cColor = cAlbedoColor + cSpecularColor + cMetallicColor + cEmissionColor;
	if (gnTexturesMask & MATERIAL_NORMAL_MAP)
	{
		float3x3 TBN = float3x3(normalize(input.tangentW), normalize(input.bitangentW), normalize(input.normalW));
		float3 vNormal = normalize(cNormalColor.rgb * 2.0f - 1.0f); //[0, 1] → [-1, 1]
		normalW = normalize(mul(vNormal, TBN));
	}
	else
	{
		normalW = normalize(input.normalW);
	}
	float4 cIllumination = Lighting(input.positionW, normalW);
	return(lerp(cColor, cIllumination, 0.5f));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define MAX_VERTEX_INFLUENCES			4
#define SKINNED_ANIMATION_BONES			256

cbuffer cbBoneOffsets : register(b7)
{
	float4x4 gpmtxBoneOffsets[SKINNED_ANIMATION_BONES];
};

cbuffer cbBoneTransforms : register(b8)
{
	float4x4 gpmtxBoneTransforms[SKINNED_ANIMATION_BONES];
};

struct VS_SKINNED_STANDARD_INPUT
{
	float3 position : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 bitangent : BITANGENT;
	int4 indices : BONEINDEX;
	float4 weights : BONEWEIGHT;
};

VS_STANDARD_OUTPUT VSSkinnedAnimationStandard(VS_SKINNED_STANDARD_INPUT input)
{
	VS_STANDARD_OUTPUT output;

	//output.positionW = float3(0.0f, 0.0f, 0.0f);
	//output.normalW = float3(0.0f, 0.0f, 0.0f);
	//output.tangentW = float3(0.0f, 0.0f, 0.0f);
	//output.bitangentW = float3(0.0f, 0.0f, 0.0f);
	//matrix mtxVertexToBoneWorld;
	//for (int i = 0; i < MAX_VERTEX_INFLUENCES; i++)
	//{
	//	mtxVertexToBoneWorld = mul(gpmtxBoneOffsets[input.indices[i]], gpmtxBoneTransforms[input.indices[i]]);
	//	output.positionW += input.weights[i] * mul(float4(input.position, 1.0f), mtxVertexToBoneWorld).xyz;
	//	output.normalW += input.weights[i] * mul(input.normal, (float3x3)mtxVertexToBoneWorld);
	//	output.tangentW += input.weights[i] * mul(input.tangent, (float3x3)mtxVertexToBoneWorld);
	//	output.bitangentW += input.weights[i] * mul(input.bitangent, (float3x3)mtxVertexToBoneWorld);
	//}
	float4x4 mtxVertexToBoneWorld = (float4x4)0.0f;
	for (int i = 0; i < MAX_VERTEX_INFLUENCES; i++)
	{
//		mtxVertexToBoneWorld += input.weights[i] * gpmtxBoneTransforms[input.indices[i]];
		mtxVertexToBoneWorld += input.weights[i] * mul(gpmtxBoneOffsets[input.indices[i]], gpmtxBoneTransforms[input.indices[i]]);
	}
	output.positionW = mul(float4(input.position, 1.0f), mtxVertexToBoneWorld).xyz;
	output.normalW = mul(input.normal, (float3x3)mtxVertexToBoneWorld).xyz;
	output.tangentW = mul(input.tangent, (float3x3)mtxVertexToBoneWorld).xyz;
	output.bitangentW = mul(input.bitangent, (float3x3)mtxVertexToBoneWorld).xyz;

//	output.positionW = mul(float4(input.position, 1.0f), gmtxGameObject).xyz;

	output.position = mul(mul(float4(output.positionW, 1.0f), gmtxView), gmtxProjection);
	output.uv = input.uv;

	return(output);
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
Texture2D gtxtTerrainBaseTexture : register(t1);
Texture2D gtxtTerrainDetailTexture : register(t2);

struct VS_TERRAIN_INPUT
{
	float3 position : POSITION;
	float4 color : COLOR;
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
};

struct VS_TERRAIN_OUTPUT
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
};

VS_TERRAIN_OUTPUT VSTerrain(VS_TERRAIN_INPUT input)
{
	VS_TERRAIN_OUTPUT output;

	output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
	output.color = input.color;
	output.uv0 = input.uv0;
	output.uv1 = input.uv1;

	return(output);
}

float4 PSTerrain(VS_TERRAIN_OUTPUT input) : SV_TARGET
{
	float4 cBaseTexColor = gtxtTerrainBaseTexture.Sample(gssWrap, input.uv0);
	float4 cDetailTexColor = gtxtTerrainDetailTexture.Sample(gssWrap, input.uv1);
//	float4 cColor = saturate((cBaseTexColor * 0.5f) + (cDetailTexColor * 0.5f));
	float4 cColor = input.color * saturate((cBaseTexColor * 0.5f) + (cDetailTexColor * 0.5f));

	return(cColor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct VS_SKYBOX_CUBEMAP_INPUT
{
	float3 position : POSITION;
};

struct VS_SKYBOX_CUBEMAP_OUTPUT
{
	float3	positionL : POSITION;
	float4	position : SV_POSITION;
};

VS_SKYBOX_CUBEMAP_OUTPUT VSSkyBox(VS_SKYBOX_CUBEMAP_INPUT input)
{
	VS_SKYBOX_CUBEMAP_OUTPUT output;

	output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
	output.positionL = input.position;

	return(output);
}

TextureCube gtxtSkyCubeTexture : register(t13);
SamplerState gssClamp : register(s1);

float4 PSSkyBox(VS_SKYBOX_CUBEMAP_OUTPUT input) : SV_TARGET
{
	float4 cColor = gtxtSkyCubeTexture.Sample(gssClamp, input.positionL);

	return(cColor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

Texture2D gtxtInputTextures[6] : register(t14); //Texture, NormalV, Color,Illumination ,Depth 


struct PS_MULTIPLE_RENDER_TARGETS_OUTPUT
{
	float4 f4Scene : SV_TARGET0; 

	float4 f4Texture : SV_TARGET1;
	float4 f4Normal : SV_TARGET2;
	float4 f4Position : SV_TARGET3;
	float4 f4Specular : SV_TARGET4;
	float4 f4Ambient : SV_TARGET5;
	float4 f4Diffuse : SV_TARGET6;
};

PS_MULTIPLE_RENDER_TARGETS_OUTPUT PSTexturedLightingToMultipleRTs(VS_STANDARD_OUTPUT input)
{
	PS_MULTIPLE_RENDER_TARGETS_OUTPUT output;

	output.f4Texture = gtxtAlbedoTexture.Sample(gssWrap, input.uv);

	//output.f3Normal = input.normalW = normalize(input.normalW);
	input.normalW = normalize(input.normalW);
	//output.f3Normal = gtxtNormalTexture.Sample(gssWrap, input.uv);
	output.f4Normal = float4(input.normalW, 0);

	//float4 Illumination = Lighting(input.positionW, input.normalW);

	input.positionW.x = (input.positionW.x - POSITION_MIN) / (POSITION_MAX - POSITION_MIN);
	input.positionW.y = (input.positionW.y - POSITION_MIN) / (POSITION_MAX - POSITION_MIN);
	input.positionW.z = (input.positionW.z - POSITION_MIN) / (POSITION_MAX - POSITION_MIN);
	output.f4Position = float4(input.positionW, 0);

	output.f4Specular = gtxtSpecularTexture.Sample(gssWrap, input.uv);
	output.f4Ambient = gMaterial.m_cAmbient;
	output.f4Diffuse = gMaterial.m_cDiffuse;

	//output.f4Illumination = Lighting(input.positionW, input.normalW);
	

	output.f4Scene = output.f4Texture;

	return(output);
}




struct VS_SCREEN_RECT_TEXTURED_OUTPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float3 viewSpaceDir : TEXCOORD1;
};

VS_SCREEN_RECT_TEXTURED_OUTPUT VSScreenRectSamplingTextured(uint nVertexID : SV_VertexID)
{
	VS_SCREEN_RECT_TEXTURED_OUTPUT output = (VS_SCREEN_RECT_TEXTURED_OUTPUT)0;

	if (nVertexID == 0) { output.position = float4(-1.0f, +1.0f, 0.0f, 1.0f); output.uv = float2(0.0f, 0.0f); }
	else if (nVertexID == 1) { output.position = float4(+1.0f, +1.0f, 0.0f, 1.0f); output.uv = float2(1.0f, 0.0f); }
	else if (nVertexID == 2) { output.position = float4(+1.0f, -1.0f, 0.0f, 1.0f); output.uv = float2(1.0f, 1.0f); }

	else if (nVertexID == 3) { output.position = float4(-1.0f, +1.0f, 0.0f, 1.0f); output.uv = float2(0.0f, 0.0f); }
	else if (nVertexID == 4) { output.position = float4(+1.0f, -1.0f, 0.0f, 1.0f); output.uv = float2(1.0f, 1.0f); }
	else if (nVertexID == 5) { output.position = float4(-1.0f, -1.0f, 0.0f, 1.0f); output.uv = float2(0.0f, 1.0f); }

	output.viewSpaceDir = mul(output.position, gmtxProjection).xyz;

	return(output);
}

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

	return((gLights[nIndex].m_cAmbient * vAmbient) + (gLights[nIndex].m_cDiffuse * fDiffuseFactor * vDiffuse) + (gLights[nIndex].m_cSpecular * fSpecularFactor * vSpecular));
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

		return(((gLights[nIndex].m_cAmbient * vAmbient) + (gLights[nIndex].m_cDiffuse * fDiffuseFactor * vDiffuse) + (gLights[nIndex].m_cSpecular * fSpecularFactor * vSpecular)) * fAttenuationFactor);
	}
	return(float4(0.0f, 0.0f, 0.0f, 0.0f));
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

		return(((gLights[nIndex].m_cAmbient * vAmbient) + (gLights[nIndex].m_cDiffuse * fDiffuseFactor * vDiffuse) + (gLights[nIndex].m_cSpecular * fSpecularFactor * vSpecular)) * fAttenuationFactor * fSpotFactor);
	}
	return(float4(0.0f, 0.0f, 0.0f, 0.0f));
}

float4 DeferredLighting(float3 vPosition, float3 vNormal, float4 vSpecular, float4 vDiffuse, float4 vAmbient)
{
	float3 vCameraPosition = float3(gvCameraPosition.x, gvCameraPosition.y, gvCameraPosition.z);
	//float3 vCameraPosition = float3(0.0f, 0.0f, 0.0f);
	float3 vToCamera = normalize(vCameraPosition - vPosition);

	float4 cColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	[unroll(MAX_LIGHTS)] for (int i = 0; i < gnLights; i++)
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

	return(cColor);
}

float4 DeferredLighting2(float3 vPosition, float3 vNormal, float4 vSpecular, float4 vDiffuse)
{
	float3 vCameraPosition = float3(gvCameraPosition.x, gvCameraPosition.y, gvCameraPosition.z);
	//float3 vCameraPosition = float3(0.0f, 0.0f, 0.0f);
	float3 vToCamera = normalize(vCameraPosition - vPosition);

	float4 cColor = float4(0.0f, 0.0f, 0.0f, 0.0f);

	float3 f3Specular = vSpecular.rgb;
	[unroll(MAX_LIGHTS)] for (int i = 0; i < gnLights; i++)
	{
		if (gLights[i].m_bEnable)
		{
			if (gLights[i].m_nType == DIRECTIONAL_LIGHT)
			{
				cColor += float4(ComputeDirectionalLight(gLights[i], f3Specular, vDiffuse, vNormal, vToCamera),1.0f);
			}
			else if (gLights[i].m_nType == POINT_LIGHT)
			{
				cColor += float4(ComputePointLight(gLights[i], f3Specular, vDiffuse, vPosition, vNormal, vToCamera), 1.0f);
			}
			else if (gLights[i].m_nType == SPOT_LIGHT)
			{
				cColor += float4(ComputeSpotLight(gLights[i], f3Specular, vDiffuse, vPosition, vNormal, vToCamera), 1.0f);
			}
		}
	}
	cColor += (gcGlobalAmbientLight * float4(1.0f, 1.0f, 1.0f, 1.0f));
	//cColor.a = vDiffuse.a;
	cColor.a = 1.0f;

	return(cColor);
}

float4 cross(float4 v1, float4 v2, float4 v3, float4 v4)
{
	float4 res;
	res.x = v2.y * v3.z * v4.w - v2.y * v4.z * v3.w - v3.y * v2.z * v4.w +
		v3.y * v4.z * v2.w + v4.y * v2.z * v3.w - v4.y * v3.z * v2.w;
	res.y = -v1.y * v3.z * v4.w + v1.y * v4.z * v3.w + v3.y * v1.z * v4.w -
		v3.y * v4.z * v1.w - v4.y * v1.z * v3.w + v4.y * v3.z * v1.w;
	res.z = v1.y * v2.z * v4.w - v1.y * v4.z * v2.w - v2.y * v1.z * v4.w +
		v2.y * v4.z * v1.w + v4.y * v1.z * v2.w - v4.y * v2.z * v1.w;
	res.w = -v1.y * v2.z * v3.w + v1.y * v3.z * v2.w + v2.y * v1.z * v3.w -
		v2.y * v3.z * v1.w - v3.y * v1.z * v2.w + v3.y * v2.z * v1.w;
	return res;
}

float4 dots(float4 v1, float4 v2)
{
	float4 res = v1 * v2;
	res = res.x + res.yxxx + res.zxyy + res.zwzw;
	return res;
}

float4x4 InverseMatrix(float4x4 m)
{
	float4 c0 = m[0];
	float4 c1 = m[1];
	float4 c2 = m[2];
	float4 c3 = m[3];

	float4 r0 = cross(c1, c2, c3, c3);
	float4 r1 = cross(c2, c3, c0, c0);
	float4 r2 = cross(c3, c0, c1, c1);
	float4 r3 = cross(c0, c1, c2, c2);

	float4 det = dots(c0, r0);

	float4x4 inv = { r0 / det.x, r1 / det.x, r2 / det.x, r3 / det.x };
	return inv;
}


float4 PSScreenRectSamplingTextured(VS_SCREEN_RECT_TEXTURED_OUTPUT input) : SV_Target
{
	//float4 cColor = gtxtInputTextures[2].Sample(gssWrap, input.uv);
	//float4 posH = float4(2.0f * input.uv.x - 1.0f, 1.0f - 2.0f * input.uv.y, 0.0f, 1.0f); // 정규화된 스크린 좌표계로 변환
	//uint3 texCoord = uint3(posH.xy, 0);

	float2 textureSize = float2(640, 480);
	float2 texelCoord = input.uv * textureSize;
	uint3 texCoord = uint3(texelCoord, 0);

	float4 Texture = gtxtInputTextures[0].Load(texCoord);
	float3 Normal = gtxtInputTextures[1].Load(texCoord).rgb;
	//float3 Position = gtxtInputTextures[2].Load(texCoord).rgb;
	float4 Position = gtxtInputTextures[2].Load(texCoord);
	float4 Specular = gtxtInputTextures[3].Load(texCoord);
	float4 Ambient = gtxtInputTextures[4].Load(texCoord);
	float4 Diffuse = gtxtInputTextures[5].Load(texCoord);

	Position.x = Position.x * (POSITION_MAX - POSITION_MIN) + POSITION_MIN;
	Position.y = Position.y * (POSITION_MAX - POSITION_MIN) + POSITION_MIN;
	Position.z = Position.z * (POSITION_MAX - POSITION_MIN) + POSITION_MIN; //문제점 aliasing
	float3 pos = Position.xyz;

	//float3 pos = mul(mul(Position, InverseMatrix(gmtxView)), InverseMatrix(gmtxProjection)).xyz;

	//float4 Illumination = DeferredLighting(Position, Normal, Specular, Diffuse, float4(1,1,1,1));
	float4 Illumination = DeferredLighting(pos, Normal, Specular, Diffuse, Ambient);
	//float4 Illumination = DeferredLighting2(Position, Normal, Specular,Diffuse);

	float4 cColor = Texture * Illumination;
	if (cColor.b >= 0.3f && cColor.r <= 0.1f && cColor.g <= 0.1f)
		discard;
	/*if(cColor.b >= 0.3f)
		cColor = gtxtSkyCubeTexture.Sample(gssClamp, input.viewSpaceDir);*/
	cColor.a = 1.0f;
	//float4 cColor = gtxtInputTextures[5].Sample(gssWrap, input.uv);

	return(cColor);
}