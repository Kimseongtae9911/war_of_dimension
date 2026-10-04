#include "Common.hlsl"

struct VS_ShadowMap_In
{
    float3 PosL : POSITION;
	//float2 TexC    : TEXCOORD;
};

struct VS_ShadowMap_Out
{
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD;
};

VS_ShadowMap_Out VSShadowMap(VS_ShadowMap_In vin)
{
    VS_ShadowMap_Out vout = (VS_ShadowMap_Out) 0.0f;

	//MaterialData matData = gMaterialData[gMaterialIndex];

	// Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gmtxGameObject);

	// Transform to homogeneous clip space.
    vout.PosH = mul(mul(posW, gmtxView), gmtxProjection);
	// Output vertex attributes for interpolation across triangle.
	/*float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = mul(texC, matData.MatTransform).xy;*/
	
    return vout;
}

// This is only used for alpha cut out geometry, so that shadows 
// show up correctly.  Geometry that does not need to sample a
// texture can use a NULL pixel shader for depth pass.
void PSShadowMap(VS_ShadowMap_Out pin)
{
	//	// Fetch the material data.
	//	MaterialData matData = gMaterialData[gMaterialIndex];
	//	float4 diffuseAlbedo = matData.DiffuseAlbedo;
	//	uint diffuseMapIndex = matData.DiffuseMapIndex;
	//
	//	// Dynamically look up the texture in the array.
	//	diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);
	//
	//#ifdef ALPHA_TEST
	//	// Discard pixel if texture alpha < 0.1.  We do this test as soon 
	//	// as possible in the shader so that we can potentially exit the
	//	// shader early, thereby skipping the rest of the shader code.
	//	clip(diffuseAlbedo.a - 0.1f);
	//#endif
}