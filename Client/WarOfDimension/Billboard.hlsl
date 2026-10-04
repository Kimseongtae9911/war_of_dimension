#include "Common.hlsl"

VS_TEXTURED_OUTPUT VSBillboard(VS_TEXTURED_INPUT input)
{
    VS_TEXTURED_OUTPUT output;
	
    float3 newPos = input.position;
	
    output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
    output.uv = input.uv;
	
    return (output);
}

float4 PSBillboard(VS_TEXTURED_OUTPUT input) : SV_TARGET
{
    float4 cColor = gtxtAlbedoTexture.Sample(gssWrap, input.uv);

    switch (gTextureKind)
    {
        case 0:
            if (gIsClicked)
                cColor.rgb *= 0.5f;
            break;
        case 1:
            cColor.a = 0.7f;
            break;
        case 2:
            cColor.a = (input.uv.x <= gTextureValue) ? cColor.a : 0.f;
            break;
        case 3:
            cColor.a = (input.uv.x >= (1.0 - gTextureValue)) ? cColor.a : 0.f;
            break;
        default:
            break;
    }

    return (cColor);
}