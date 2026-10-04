#include "DeferredRender.hlsl"

VS_TEXTURED_OUTPUT VSTextureToViewport(uint nVertexID : SV_VertexID)
{
    VS_TEXTURED_OUTPUT output = (VS_TEXTURED_OUTPUT) 0;

    if (nVertexID == 0)
    {
        output.position = float4(-1.0f, +1.0f, 0.0f, 1.0f);
        output.uv = float2(0.0f, 0.0f);
    }
    if (nVertexID == 1)
    {
        output.position = float4(+1.0f, +1.0f, 0.0f, 1.0f);
        output.uv = float2(1.0f, 0.0f);
    }
    if (nVertexID == 2)
    {
        output.position = float4(+1.0f, -1.0f, 0.0f, 1.0f);
        output.uv = float2(1.0f, 1.0f);
    }
    if (nVertexID == 3)
    {
        output.position = float4(-1.0f, +1.0f, 0.0f, 1.0f);
        output.uv = float2(0.0f, 0.0f);
    }
    if (nVertexID == 4)
    {
        output.position = float4(+1.0f, -1.0f, 0.0f, 1.0f);
        output.uv = float2(1.0f, 1.0f);
    }
    if (nVertexID == 5)
    {
        output.position = float4(-1.0f, -1.0f, 0.0f, 1.0f);
        output.uv = float2(0.0f, 1.0f);
    }

    return (output);
}


float4 PSTextureToViewport(VS_TEXTURED_OUTPUT input) : SV_Target
{
    float2 textureSize = float2(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
    float2 texelCoord = input.uv * textureSize;
    uint3 texCoord = uint3(texelCoord, 0);

    float4 Texture = gtxtInputTextures[0].Load(texCoord);

    float4 cColor = Texture;
    cColor = gtxtInputTextures[0].Sample(gssWrap, input.uv);
	
    return (cColor);
}

VS_TEXTURED_OUTPUT VSDebugDiffuseToViewport(uint nVertexID : SV_VertexID)
{
    VS_TEXTURED_OUTPUT output = (VS_TEXTURED_OUTPUT) 0;

    if (nVertexID == 0)
    {
        output.position = float4(-1.0f, +1.0f, 0.0f, 1.0f);
        output.uv = float2(0.0f, 0.0f);
    }
    if (nVertexID == 1)
    {
        output.position = float4(+1.0f, +1.0f, 0.0f, 1.0f);
        output.uv = float2(1.0f, 0.0f);
    }
    if (nVertexID == 2)
    {
        output.position = float4(+1.0f, -1.0f, 0.0f, 1.0f);
        output.uv = float2(1.0f, 1.0f);
    }
    if (nVertexID == 3)
    {
        output.position = float4(-1.0f, +1.0f, 0.0f, 1.0f);
        output.uv = float2(0.0f, 0.0f);
    }
    if (nVertexID == 4)
    {
        output.position = float4(+1.0f, -1.0f, 0.0f, 1.0f);
        output.uv = float2(1.0f, 1.0f);
    }
    if (nVertexID == 5)
    {
        output.position = float4(-1.0f, -1.0f, 0.0f, 1.0f);
        output.uv = float2(0.0f, 1.0f);
    }

    return (output);
}


float4 PSDebugDiffuseToViewport(VS_TEXTURED_OUTPUT input) : SV_Target
{
    float2 textureSize = float2(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
    float2 texelCoord = input.uv * textureSize;
    uint3 texCoord = uint3(texelCoord, 0);

    float4 Diffuse = gtxtInputTextures[3].Load(texCoord);

    float4 cColor = float4(Diffuse.g, Diffuse.g, Diffuse.g, 1.0f);
	//float4 n = float4(gtxtDepthTextures[0].SampleLevel(gssWrap, input.uv, 0).rrr, 1.0f);
    cColor = gtxShadowMap.Sample(gssWrap, input.uv).r;
    return (cColor);
}

VS_TEXTURED_OUTPUT VSDebugTextureToViewport(uint nVertexID : SV_VertexID)
{
    VS_TEXTURED_OUTPUT output = (VS_TEXTURED_OUTPUT) 0;

    if (nVertexID == 0)
    {
        output.position = float4(-1.0f, +1.0f, 0.0f, 1.0f);
        output.uv = float2(0.0f, 0.0f);
    }
    if (nVertexID == 1)
    {
        output.position = float4(+1.0f, +1.0f, 0.0f, 1.0f);
        output.uv = float2(1.0f, 0.0f);
    }
    if (nVertexID == 2)
    {
        output.position = float4(+1.0f, -1.0f, 0.0f, 1.0f);
        output.uv = float2(1.0f, 1.0f);
    }
    if (nVertexID == 3)
    {
        output.position = float4(-1.0f, +1.0f, 0.0f, 1.0f);
        output.uv = float2(0.0f, 0.0f);
    }
    if (nVertexID == 4)
    {
        output.position = float4(+1.0f, -1.0f, 0.0f, 1.0f);
        output.uv = float2(1.0f, 1.0f);
    }
    if (nVertexID == 5)
    {
        output.position = float4(-1.0f, -1.0f, 0.0f, 1.0f);
        output.uv = float2(0.0f, 1.0f);
    }

    return (output);
}


float4 PSDebugTextureToViewport(VS_TEXTURED_OUTPUT input) : SV_Target
{
    float2 textureSize = float2(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
    float2 texelCoord = input.uv * textureSize;
    uint3 texCoord = uint3(texelCoord, 0);

    float4 Texture = gtxtInputTextures[0].Load(texCoord);

    float4 cColor = Texture;
    cColor = gtxtInputTextures[0].Sample(gssWrap, input.uv);
	
    return (cColor);
}


VS_TEXTURED_OUTPUT VSDebugLightingToViewport(uint nVertexID : SV_VertexID)
{
    VS_TEXTURED_OUTPUT output = (VS_TEXTURED_OUTPUT) 0;

    if (nVertexID == 0)
    {
        output.position = float4(-1.0f, +1.0f, 0.0f, 1.0f);
        output.uv = float2(0.0f, 0.0f);
    }
    if (nVertexID == 1)
    {
        output.position = float4(+1.0f, +1.0f, 0.0f, 1.0f);
        output.uv = float2(1.0f, 0.0f);
    }
    if (nVertexID == 2)
    {
        output.position = float4(+1.0f, -1.0f, 0.0f, 1.0f);
        output.uv = float2(1.0f, 1.0f);
    }
    if (nVertexID == 3)
    {
        output.position = float4(-1.0f, +1.0f, 0.0f, 1.0f);
        output.uv = float2(0.0f, 0.0f);
    }
    if (nVertexID == 4)
    {
        output.position = float4(+1.0f, -1.0f, 0.0f, 1.0f);
        output.uv = float2(1.0f, 1.0f);
    }
    if (nVertexID == 5)
    {
        output.position = float4(-1.0f, -1.0f, 0.0f, 1.0f);
        output.uv = float2(0.0f, 1.0f);
    }

    return (output);
}


float4 PSDebugLightingToViewport(VS_TEXTURED_OUTPUT input) : SV_Target
{
    float2 textureSize = float2(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
    float2 texelCoord = input.uv * textureSize;
    uint3 texCoord = uint3(texelCoord, 0);


    float3 Normal = gtxtInputTextures[1].Load(texCoord).rgb;
    float4 Position = gtxtInputTextures[2].Load(texCoord);
    float4 Specular = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 Ambient = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 Diffuse = gtxtInputTextures[5].Load(texCoord);

    Position.x = Position.x * (POSITION_MAX - POSITION_MIN) + POSITION_MIN;
    Position.y = Position.y * (POSITION_MAX - POSITION_MIN) + POSITION_MIN;
    Position.z = Position.z * (POSITION_MAX - POSITION_MIN) + POSITION_MIN; //¹®Á¦Á¡ aliasing
    float3 pos = Position.xyz;

    float4 Illumination = DeferredLighting(pos, Normal, Specular, Diffuse, Ambient);

    float4 cColor = gtxtInputTextures[3].Load(texCoord);

    return (Illumination);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bounding Box

VS_TEXTURED_OUTPUT VSBoxTextured(VS_TEXTURED_INPUT input)
{
    VS_TEXTURED_OUTPUT output;

    output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
    output.uv = input.uv;

    return (output);
}

float4 PSBoxTextured(VS_TEXTURED_OUTPUT input, uint nPrimitiveID : SV_PrimitiveID) : SV_TARGET
{
    float3 uvw = float3(input.uv, nPrimitiveID / 2);
    float4 cColor = float4(0, 0, 1, 1);

    return (cColor);
}