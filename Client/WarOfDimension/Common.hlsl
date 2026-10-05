#include "Light.hlsl"

#define POSITION_MAX 1000.f
#define POSITION_MIN -1000.f

#define FRAME_BUFFER_WIDTH				1920
#define FRAME_BUFFER_HEIGHT				1080

//#define _WITH_VERTEX_LIGHTING

#define MATERIAL_ALBEDO_MAP			0x01
#define MATERIAL_SPECULAR_MAP		0x02
#define MATERIAL_NORMAL_MAP			0x04
#define MATERIAL_METALLIC_MAP		0x08
#define MATERIAL_EMISSION_MAP		0x10
#define MATERIAL_DETAIL_ALBEDO_MAP	0x20
#define MATERIAL_DETAIL_NORMAL_MAP	0x40

#define MAX_VERTEX_INFLUENCES			4
#define SKINNED_ANIMATION_BONES			256

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Structs

cbuffer cbSceneState : register(b3)
{
	int gDrawOption;	// HDR
	float exposure;		// color grading
	float saturation;	// color grading
	float contrast;		// color grading
	float vibrance;		// color grading
    int gCurScene; // current scene
    bool gOutline;
}

struct CB_TOOBJECTSPACE
{
	matrix		mtxToTexture;
	float4		f4Position;
};

cbuffer cbToLightSpace : register(b5)
{
	CB_TOOBJECTSPACE gcbToLightSpaces[MAX_LIGHTS];
};

cbuffer cbFrameworkInfo : register(b6)
{
    float gCurrentTime;
	float gElapsedTime;
}

cbuffer cbBoneOffsets : register(b7)
{
	float4x4 gpmtxBoneOffsets[SKINNED_ANIMATION_BONES];
};

cbuffer cbBoneTransforms : register(b8)
{
	float4x4 gpmtxBoneTransforms[SKINNED_ANIMATION_BONES]; 
};

cbuffer cbMeshInfo : register(b9)
{
	bool gIsClicked;
	int gTextureKind;
    float gTextureValue;
    float gTextureUVx; // UVOffset for Skill Icon Texture, Kind 5, 6
    float gTextureUVy; // UVOffset for Skill Icon Texture, Kind 5, 6
    float3 gUiPadding;
    float4 gUiUvTransform;
}

cbuffer cbParticleForwardVector : register(b11)
{
    float4 gvColor;
    float3 gvForwardVector;
    float gfParticleSize;
    float gfLifeTime;
    int giParticleNum;
    int gTotalSpriteNum;
    int gWidthSpriteNum;
    int gCurrentSpriteNum;
    int giboolLean;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Textures

// Terrain Textures
Texture2D gtxtTerrainBaseTexture : register(t1);
Texture2D gtxtTerrainDetailTexture : register(t2);

// Standard Shader Texture
Texture2D gtxtAlbedoTexture : register(t6);
Texture2D gtxtSpecularTexture : register(t7);
Texture2D gtxtNormalTexture : register(t8);
Texture2D gtxtMetallicTexture : register(t9);
Texture2D gtxtEmissionTexture : register(t10);
Texture2D gtxtDetailAlbedoTexture : register(t11);
Texture2D gtxtDetailNormalTexture : register(t12);

Texture2D gtxtDissolveTexture : register(t26);

// Skybox Texture
TextureCube gtxtSkyCubeTexture : register(t13);

// Skill BillboardTexture
Texture2D<float4> gtxtBillboardTextures[4] : register(t14);
Texture2D gtxtInputTextures[6] : register(t18); //To Defferd Rendering
Texture2D gtxShadowMap : register(t25);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sampler States

SamplerState gssWrap : register(s0);
SamplerState gssClamp : register(s1);
SamplerState gssBorder : register(s3);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global Functions

// Durand Tone Mapping, Basic Method, simply calcul lum val
float3 DurandToneMapping(float3 color)
{
	float3 lum = dot(color, float3(0.2126, 0.7152, 0.0722));
	float3 result = (color / (1.0 + lum)) * (1.0 + (lum / (1.0 + lum)) * (1.0 - (1.0 / (lum + 1.0))));
	return result;
}

// Filmic Uncharted2 Tone Mapping Technique
float3 Uncharted2TonemapPartial(float3 x)
{
	float A = 0.15f;
	float B = 0.50f;
	float C = 0.10f;
	float D = 0.20f;
	float E = 0.02f;
	float F = 0.30f;

	float3 curr = x;
	curr = (curr * (A * curr + C * B) + D * E) / (curr * (A * curr + B) + D * F);
	return curr;
}

float3 Uncharted2Tonemap(float3 x)	//Filmic Effect
{
	float exposure_bias = 2.0f;
	float3 curr = Uncharted2TonemapPartial(x * exposure_bias);

	float3 W = float3(11.2f, 11.2f, 11.2f);
	float3 white_scale = float3(1.0f, 1.0f, 1.0f) / Uncharted2TonemapPartial(W);
	return curr * white_scale;
}

// ACES(Academy Color Encoding System), Tone mapping technique used by Unreal Engine 4
static const float3x3 aces_input_matrix = {
	float3(0.59719f, 0.35458f, 0.04823f),
	float3(0.07600f, 0.90834f, 0.01566f),
	float3(0.02840f, 0.13383f, 0.83777f)
};

static const float3x3 aces_output_matrix = {
	float3(1.60475f, -0.53108f, -0.07367f),
	float3(-0.10208f, 1.10813f, -0.00605f),
	float3(-0.00327f, -0.07276f, 1.07602f)
};

float3 MultiplyMatrix(float3x3 m, float3 v)
{
	float x = m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2];
	float y = m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2];
	float z = m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2];

	return float3(x, y, z);
}

float3 RttAndOdtFit(float3 v)
{
	float3 a = v * (v + 0.0245786f) - 0.000090537f;
	float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
	return a / b;
}

float3 ACESFitted(float3 v)
{
	v = MultiplyMatrix(aces_input_matrix, v);
	v = RttAndOdtFit(v);
	return mul(aces_output_matrix, v);
}

// HableMcCann Tone mapping
float3 HableMcCannToneMapping(float3 color, float exposure, float3 whitePoint)
{
	// Parameters for Hable-McCann tone mapping
	float3 a = float3(0.22, 0.30, 0.10);
	float3 b = float3(0.01, 0.10, 0.20);
	float3 c = float3(0.20, 0.01, 0.30);
	float3 d = float3(0.02, 0.30, 0.11);
	float3 e = float3(0.30, 0.00, 0.00);
	float3 f = float3(0.30, 0.00, 0.00);

	// Apply exposure
	color *= exposure;

	// Calculate the luminance
	float Y = dot(color, a) + dot(color * color, b) + dot(color * color * color, c);

	// Calculate the white scale
	float W = dot(whitePoint, a) + dot(whitePoint * whitePoint, b) + dot(whitePoint * whitePoint * whitePoint, c);
	float whiteScale = W / Y;

	// Calculate the saturation
	float3 colorRatio = (color * (d * color + e)) / (color * (f * color + float3(1.0, 1.0, 1.0)));
	float saturation = max(colorRatio.r, max(colorRatio.g, colorRatio.b));

	// Apply the tone mapping
	float3 mappedColor = color * (1.0 + color * whiteScale) / (1.0 + color);

	// Adjust the contrast and brightness
	float contrastFactor = 2.0;
	float brightnessFactor = 0.95;
	mappedColor = pow(mappedColor, float3(contrastFactor, contrastFactor, contrastFactor));
	mappedColor *= brightnessFactor;

	// Adjust the saturation
	mappedColor = lerp(float3(Y, Y, Y), mappedColor, saturation);

	return mappedColor;
}

float3 HableMcCannToneMappingAutoExposure(float3 color, float3 whitePoint, float exposure)
{
	// Parameters for Hable-McCann tone mapping
	float3 a = float3(0.22, 0.30, 0.10);
	float3 b = float3(0.01, 0.10, 0.20);
	float3 c = float3(0.20, 0.01, 0.30);
	float3 d = float3(0.02, 0.30, 0.11);
	float3 e = float3(0.30, 0.00, 0.00);
	float3 f = float3(0.30, 0.00, 0.00);

	// Apply exposure
	color *= pow(2.0, exposure);

	// Calculate the luminance
	float Y = dot(color, a) + dot(color * color, b) + dot(color * color * color, c);

	// Calculate the white scale
	float W = dot(whitePoint, a) + dot(whitePoint * whitePoint, b) + dot(whitePoint * whitePoint * whitePoint, c);
	float whiteScale = W / Y;

	// Calculate the saturation
	float3 colorRatio = (color * (d * color + e)) / (color * (f * color + float3(1.0, 1.0, 1.0)));
	float saturation = max(colorRatio.r, max(colorRatio.g, colorRatio.b));

	// Apply the tone mapping
	float3 mappedColor = color * (1.0 + color * whiteScale) / (1.0 + color);

	// Adjust the contrast and brightness
	float contrastFactor = 2.0;
	float brightnessFactor = 0.95;
	mappedColor = pow(mappedColor, float3(contrastFactor, contrastFactor, contrastFactor));
	mappedColor *= brightnessFactor;

	// Adjust the saturation
	mappedColor = lerp(float3(Y, Y, Y), mappedColor, saturation);

	return mappedColor;
}

// Sobel Outline Compute
float4 Sobel(Texture2D tex, float2 uv, SamplerState sam)
{
    float2 texelSize = 1.0f / float2(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);

	float4 tl = tex.Sample(sam, uv + float2(-texelSize.x, -texelSize.y));
	float4 tc = tex.Sample(sam, uv + float2(0, -texelSize.y));
	float4 tr = tex.Sample(sam, uv + float2(texelSize.x, -texelSize.y));
	float4 cl = tex.Sample(sam, uv + float2(-texelSize.x, 0));
	float4 cr = tex.Sample(sam, uv + float2(texelSize.x, 0));
	float4 bl = tex.Sample(sam, uv + float2(-texelSize.x, texelSize.y));
	float4 bc = tex.Sample(sam, uv + float2(0, texelSize.y));
	float4 br = tex.Sample(sam, uv + float2(texelSize.x, texelSize.y));

	float4 gx = -1 * tl + -2 * cl + -1 * bl + 1 * tr + 2 * cr + 1 * br;
	float4 gy = -1 * tl + -2 * tc + -1 * tr + 1 * bl + 2 * bc + 1 * br;

	float4 g = sqrt(gx * gx + gy * gy);

	return g;
}

// Color Grading
float4 ColorGrading(float4 color, float exposure, float saturation, float contrast, float vibrance)
{
	// Adjust the exposure
	color.rgb *= exposure;

	// Adjust the saturation and vibrance
	float grey = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
	float3 diff = color.rgb - grey;
	float3 t = (diff * (saturation) + grey);
	color.rgb = lerp(grey, t, vibrance);

	// Adjust the contrast
	color.rgb = ((color.rgb - 0.5f) * max(contrast, 0.0f)) + 0.5f;

	return color;
}

// Calculate rim lighting
float3 RimLighting(float3 color, float3 pos, float3 normal, float3 rimColor)
{
	float rim = 1.0f - saturate(dot(normalize(gvCameraPosition.xyz - pos), normal));
	float4 cRimColor = float4(rimColor.x, rimColor.g, rimColor.b, 1.0f);
	color += cRimColor.rgb * pow(rim, 400.0f);
	return color;
}

// Calculate Motion Blur
float4 MotionBlur(float2 uv, float2 velocity, float blurAmount, float deltaTime)
{
	// Calculate the previous pixel position
	float2 prevUV = uv - velocity * blurAmount;

	// Sample the current and previous pixel colors
	float4 currColor = gtxtInputTextures[0].Sample(gssWrap, uv);
	float4 prevColor = gtxtInputTextures[0].Sample(gssWrap, prevUV);

	// Calculate the blur weight
	float weight = deltaTime / blurAmount;

	// Average the current and previous pixel colors
	float4 blurColor = lerp(prevColor, currColor, weight);

	// Add the blurred color to the final output
	return blurColor;
}

float2 WaveDistortion(float2 uv, float freq, float amp, float time)
{
	float2 offset = float2(sin(uv.y * freq + time), cos(uv.x * freq + time));
	offset *= amp;
	return uv + offset;
}

float4 ExponentialHeightFog(float4 originColor, float3 position, float3 viewDirection)
{
	// Fog parameters
	float FogDensity = 0.01f;  // Adjust this value to control the density of the fog
	float FogHeight = 10.0f;  // Adjust this value to control the height at which the fog starts

	float distance = length(position.xyz - gvCameraPosition.xyz);  // Calculate the distance from the camera position
	float fogFactor = 1.0f - exp(-FogDensity * (distance - FogHeight));

	// Apply fog color to the pixel
	float3 fogColor = float3(0.5f, 0.5f, 0.5f);  // Set the desired fog color
	float3 originalColor = originColor.rgb;  // Store the original color
	float3 finalColor = lerp(originalColor, fogColor, fogFactor);

	return float4(finalColor, originColor.a);
}

static float gfGaussianBlurMask2D[5][5] = {
	{ 1.0f / 273.0f, 4.0f / 273.0f, 7.0f / 273.0f, 4.0f / 273.0f, 1.0f / 273.0f },
	{ 4.0f / 273.0f, 16.0f / 273.0f, 26.0f / 273.0f, 16.0f / 273.0f, 4.0f / 273.0f },
	{ 7.0f / 273.0f, 26.0f / 273.0f, 41.0f / 273.0f, 26.0f / 273.0f, 7.0f / 273.0f },
	{ 4.0f / 273.0f, 16.0f / 273.0f, 26.0f / 273.0f, 16.0f / 273.0f, 4.0f / 273.0f },
	{ 1.0f / 273.0f, 4.0f / 273.0f, 7.0f / 273.0f, 4.0f / 273.0f, 1.0f / 273.0f }
};


// atlas/progress의 논리 UV 계산을 유지한 뒤에만 패딩 내용을 매핑한다.
float2 UiTextureCoordinates(float2 uv, float4 transform)
{
    if (all(transform == float4(1, 1, 0, 0))) return uv;
    return frac(uv) * transform.xy + transform.zw;
}

float4 GaussianBlur(float2 texCoord, float blurStrength, float4 uvTransform = float4(1, 1, 0, 0))
{
    const int maxBlurRadius = 10;
    int blurRadius = int(blurStrength * maxBlurRadius);; // Radius of the blur kernel
    blurRadius = clamp(blurRadius, 0, maxBlurRadius);
	
	// Accumulate the weighted sum of colors
	float4 blurredColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float totalWeight = 0.0f;
	float2 textureSize = float2(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT); // Dimensions of the input texture
	float2 pixelSize = 1.0 / textureSize;

	// Sample the input texture multiple times within the blur radius
	for (int x = -blurRadius; x <= blurRadius; x++)
	{
		for (int y = -blurRadius; y <= blurRadius; y++)
		{
			float2 offset = float2(x, y) * pixelSize;
			float2 uv = texCoord + offset;

			float4 color = gtxtAlbedoTexture.Sample(gssWrap, UiTextureCoordinates(uv, uvTransform));

			// Calculate the Gaussian weight based on the distance from the center pixel
			float distance = length(offset);
			float weight = exp(-(distance * distance) / (2 * blurRadius * blurRadius));

			blurredColor += color * weight;
			totalWeight += weight;
		}
	}

	// Normalize the accumulated color by the total weight
	blurredColor /= totalWeight;

	return blurredColor;
}

float2 GenerateOffset(int index, int kernelSize, float texelSize)
{
    int halfSize = kernelSize / 2;
    int x = index % kernelSize;
    int y = index / kernelSize;
    return float2((x - halfSize) * texelSize, (y - halfSize) * texelSize);
}

float CalcShadowFactor(float4 shadowPosH, float bias)
{

    shadowPosH.xyz /= shadowPosH.w;

    float depth = shadowPosH.z - 0.001;

    uint width, height, numMips;
    gtxShadowMap.GetDimensions(0, width, height, numMips);

    // Texel size.
    float dx = 1.0f / (float) width;
    int kernelSize = 3;
    
    float percentLit = 0.0f;
    
    [unroll]
    for (int i = 0; i < kernelSize * kernelSize; ++i)
    {
        float2 offset = GenerateOffset(i, kernelSize, dx);
        percentLit += gtxShadowMap.SampleCmpLevelZero(gssComparisonPCFShadow, shadowPosH.xy + offset, depth + bias).r;
    }

    return percentLit / (float) (kernelSize * kernelSize);

}

float Twinkle(float3 position, float frequency, float amplitude, bool abstract)
{
	// Calculate the twinkling effect based on time
	//frequency = 1.5f;  // Adjust this value to control the speed of twinkling
	//amplitude = 0.4f;  // Adjust this value to control the strength of twinkling

    float twinkleFactor = sin(gCurrentTime * frequency) * amplitude;
    twinkleFactor = abstract ? abs(twinkleFactor) : twinkleFactor;
    return twinkleFactor;
}

float3 RotateTextureCoordinates(float3 textureCoords, float rotationAngle)
{
	// Calculate the center of the texture (assuming texture coordinates are normalized)
    float2 center = float2(0.5, 0.5);

	// Translate the texture coordinates to be centered around the origin
    float2 translatedCoords = textureCoords.xy - center;

	// Apply rotation using a 2D rotation matrix
    float2 rotatedCoords;
    rotatedCoords.x = translatedCoords.x * cos(rotationAngle) - translatedCoords.y * sin(rotationAngle);
    rotatedCoords.y = translatedCoords.x * sin(rotationAngle) + translatedCoords.y * cos(rotationAngle);

	// Translate the texture coordinates back to the original position
    float2 finalCoords = rotatedCoords + center;

	// Combine the rotated coordinates with the original Z component
    return float3(finalCoords, textureCoords.z);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

struct VS_TEXTURED_INPUT
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_TEXTURED_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};
