//--------------------------------------------------------------------------------------
#define MAX_LIGHTS			16 
#define MAX_MATERIALS		16 

#define POINT_LIGHT			1
#define SPOT_LIGHT			2
#define DIRECTIONAL_LIGHT	3

#define _WITH_LOCAL_VIEWER_HIGHLIGHTING
#define _WITH_THETA_PHI_CONES
//#define _WITH_REFLECT
#define LIGHT_STRENGTH float3(1.0f, 1.0f, 1.0f)

struct LIGHT
{
	float4					m_cAmbient;
	float4					m_cDiffuse;
	float4					m_cSpecular;
	float3					m_vPosition;
	float 					m_fFalloff;
	float3					m_vDirection;
	float 					m_fTheta; //cos(m_fTheta)
	float3					m_vAttenuation;
	float					m_fPhi; //cos(m_fPhi)
	bool					m_bEnable;
	int 					m_nType;
	float					m_fRange;
	float					padding;
};


cbuffer cbLights : register(b4)
{
	LIGHT					gLights[MAX_LIGHTS];
	float4					gcGlobalAmbientLight;
	int						gnLights;
};

float4 DirectionalLight(int nIndex, float3 vNormal, float3 vToCamera)
{
	float3 vToLight = -gLights[nIndex].m_vDirection;
	float fDiffuseFactor = dot(vToLight, vNormal);
	float fSpecularFactor = 0.0f;
	if (fDiffuseFactor > 0.0f)
	{
		if (gMaterial.m_cSpecular.a != 0.0f)
		{
#ifdef _WITH_REFLECT
			float3 vReflect = reflect(-vToLight, vNormal);
			fSpecularFactor = pow(max(dot(vReflect, vToCamera), 0.0f), gMaterial.m_cSpecular.a);
#else
#ifdef _WITH_LOCAL_VIEWER_HIGHLIGHTING
			float3 vHalf = normalize(vToCamera + vToLight);
#else
			float3 vHalf = float3(0.0f, 1.0f, 0.0f);
#endif
			fSpecularFactor = pow(max(dot(vHalf, vNormal), 0.0f), gMaterial.m_cSpecular.a);
#endif
		}
	}

	return((gLights[nIndex].m_cAmbient * gMaterial.m_cAmbient) + (gLights[nIndex].m_cDiffuse * fDiffuseFactor * gMaterial.m_cDiffuse) + (gLights[nIndex].m_cSpecular * fSpecularFactor * gMaterial.m_cSpecular));
}

float4 PointLight(int nIndex, float3 vPosition, float3 vNormal, float3 vToCamera)
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
			if (gMaterial.m_cSpecular.a != 0.0f)
			{
#ifdef _WITH_REFLECT
				float3 vReflect = reflect(-vToLight, vNormal);
				fSpecularFactor = pow(max(dot(vReflect, vToCamera), 0.0f), gMaterial.m_cSpecular.a);
#else
#ifdef _WITH_LOCAL_VIEWER_HIGHLIGHTING
				float3 vHalf = normalize(vToCamera + vToLight);
#else
				float3 vHalf = float3(0.0f, 1.0f, 0.0f);
#endif
				fSpecularFactor = pow(max(dot(vHalf, vNormal), 0.0f), gMaterial.m_cSpecular.a);
#endif
			}
		}
		float fAttenuationFactor = 1.0f / dot(gLights[nIndex].m_vAttenuation, float3(1.0f, fDistance, fDistance*fDistance));

		return(((gLights[nIndex].m_cAmbient * gMaterial.m_cAmbient) + (gLights[nIndex].m_cDiffuse * fDiffuseFactor * gMaterial.m_cDiffuse) + (gLights[nIndex].m_cSpecular * fSpecularFactor * gMaterial.m_cSpecular)) * fAttenuationFactor);
	}
	return(float4(0.0f, 0.0f, 0.0f, 0.0f));
}

float4 SpotLight(int nIndex, float3 vPosition, float3 vNormal, float3 vToCamera)
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
			if (gMaterial.m_cSpecular.a != 0.0f)
			{
#ifdef _WITH_REFLECT
				float3 vReflect = reflect(-vToLight, vNormal);
				fSpecularFactor = pow(max(dot(vReflect, vToCamera), 0.0f), gMaterial.m_cSpecular.a);
#else
#ifdef _WITH_LOCAL_VIEWER_HIGHLIGHTING
				float3 vHalf = normalize(vToCamera + vToLight);
#else
				float3 vHalf = float3(0.0f, 1.0f, 0.0f);
#endif
				fSpecularFactor = pow(max(dot(vHalf, vNormal), 0.0f), gMaterial.m_cSpecular.a);
#endif
			}
		}
#ifdef _WITH_THETA_PHI_CONES
		float fAlpha = max(dot(-vToLight, gLights[nIndex].m_vDirection), 0.0f);
		float fSpotFactor = pow(max(((fAlpha - gLights[nIndex].m_fPhi) / (gLights[nIndex].m_fTheta - gLights[nIndex].m_fPhi)), 0.0f), gLights[nIndex].m_fFalloff);
#else
		float fSpotFactor = pow(max(dot(-vToLight, gLights[i].m_vDirection), 0.0f), gLights[i].m_fFalloff);
#endif
		float fAttenuationFactor = 1.0f / dot(gLights[nIndex].m_vAttenuation, float3(1.0f, fDistance, fDistance*fDistance));

		return(((gLights[nIndex].m_cAmbient * gMaterial.m_cAmbient) + (gLights[nIndex].m_cDiffuse * fDiffuseFactor * gMaterial.m_cDiffuse) + (gLights[nIndex].m_cSpecular * fSpecularFactor * gMaterial.m_cSpecular)) * fAttenuationFactor * fSpotFactor);
	}
	return(float4(0.0f, 0.0f, 0.0f, 0.0f));
}

float4 Lighting(float3 vPosition, float3 vNormal)
{
	float3 vCameraPosition = float3(gvCameraPosition.x, gvCameraPosition.y, gvCameraPosition.z);
	float3 vToCamera = normalize(vCameraPosition - vPosition);

	float4 cColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	[unroll(MAX_LIGHTS)] for (int i = 0; i < gnLights; i++)
	{
		if (gLights[i].m_bEnable)
		{
			if (gLights[i].m_nType == DIRECTIONAL_LIGHT)
			{
				cColor += DirectionalLight(i, vNormal, vToCamera);
			}
			else if (gLights[i].m_nType == POINT_LIGHT)
			{
				cColor += PointLight(i, vPosition, vNormal, vToCamera);
			}
			else if (gLights[i].m_nType == SPOT_LIGHT)
			{
				cColor += SpotLight(i, vPosition, vNormal, vToCamera);
			}
		}
	}
	cColor += (gcGlobalAmbientLight * gMaterial.m_cAmbient);
	cColor.a = gMaterial.m_cDiffuse.a;

	return(cColor);
}

// 프레넬 방정식의 슐릭 근사를 구한다.
// 즉, 번섭이 n인 표면에서 프레넬 효과에 의해 반사되는 빛의 비율을
// 빛 벡터 L과 표면 법선 n 사이의 각도에 근거해서 근사한다.
float3 SchlickFresnel(float3 r0, float3 normal, float3 lightVec)
{
	float cosIncidentAngle = saturate(dot(normal, lightVec));

	float f0 = 1.0f - cosIncidentAngle;
	float3 reflectPercent = r0 + (1.0f - r0) * (pow(f0, 5));

	return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, float3 vSpecular, float4 vDiffuse)
{
	const float m = 80 * 256.0f;
	float3 halfVec = normalize(toEye + lightVec);

	float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
	float3 specularFactor = SchlickFresnel(vSpecular, halfVec, lightVec);

	float3 specAlbedo = specularFactor * roughnessFactor;

	// 반영 반사율 공식이 [0,1] 구간 바깥의 값을 낼 수도 있지만,
	// 우리는 LDR 렌더링을 구현하므로, 반사율을 1미만으로 낮춘다.
	specAlbedo = specAlbedo / (specAlbedo + 1.0f);

	return (vDiffuse.rgb + specAlbedo) * lightStrength;
}

float3 ComputeDirectionalLight(LIGHT light, float3 vSpecular, float4 vDiffuse, float3 normal, float3 toEye)
{
	// 빛 벡터는 광선들이 나아가는 방향의 반대 방향을 가리킨다.
	float3 lightVec = -light.m_vDirection;

	// 람베트르 코사인 법칙에 따라 빛의 세기를 줄인다.
	float ndotl = max(dot(lightVec, normal), 0.0f);
	float3 lightStrength = LIGHT_STRENGTH * ndotl;

	return BlinnPhong(lightStrength, lightVec, normal, toEye, vSpecular, vDiffuse);
}

float3 ComputePointLight(LIGHT light, float3 vSpecular, float4 vDiffuse, float3 pos, float3 normal, float3 toEye)
{
	// 표면에서 광원으로의 벡터
	float3 lightVec = light.m_vPosition - pos;

	// 광원과 표면 사이의 거리
	float d = length(lightVec);

	//범위 판정
	if (d > light.m_fFalloff)
		return float3(0.0f, 0.0f, 0.0f);

	// 빛 벡터를 정규화한다.
	lightVec /= d;

	// 람베트르 코사인 법칙에 따라 빛의 세기를 줄인다.
	float ndotl = max(dot(lightVec, normal), 0.0f);
	float3 lightStrength = LIGHT_STRENGTH * ndotl;

	// 거리에 따라 빛을 감쇠한다.
	//float att = CalcAttenuation(d, light.falloffStart, light.falloffEnd);
	//lightStrength = lightStrength * att;

	return BlinnPhong(lightStrength, lightVec, normal, toEye, vSpecular, vDiffuse);
}

float3 ComputeSpotLight(LIGHT light, float3 vSpecular, float4 vDiffuse, float3 pos, float3 normal, float3 toEye)
{
	// 표면에서 광원으로의 벡터
	float3 lightVec = light.m_vPosition - pos;

	// 광원과 표면 사이의 거리
	float d = length(lightVec);

	// 범위 판정
	if (d > light.m_fFalloff)
		return float3(0.0f, 0.0f, 0.0f);

	// 빛 벡터를 정규화한다.
	lightVec /= d;

	// 람베트르 코사인 법칙에 따라 빛의 세기를 줄인다.
	float ndotl = max(dot(lightVec, normal), 0.0f);
	float3 lightStrength = LIGHT_STRENGTH * ndotl;

	// 거리에 따라 빛을 감쇠한다.
	//float att = CalcAttenuation(d, light.falloffStart, light.falloffEnd);
	//lightStrength *= att;

	float minCos = cos(radians(light.m_fPhi));
	float maxCos = lerp(minCos, 1.0f, 0.5f);
	float cosAngle = max(dot(-lightVec, light.m_vDirection), 0.0f);

	// Angle에 따라 빛을 감쇠한다.
	float spotIntensity = smoothstep(minCos, maxCos, cosAngle);
	lightStrength *= spotIntensity;

	return BlinnPhong(lightStrength, lightVec, normal, toEye, vSpecular, vDiffuse);
}