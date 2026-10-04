#include "Common.hlsl"

float4 PSBlur(float4 position : SV_POSITION) : SV_Target
{
    float4 cColor = gtxtAlbedoTexture[int2(position.xy)];
	  // Convert the screen-space position to texture coordinates (uv)
    float2 texCoord = position.xy / float2(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);

    float4 objInfo = gtxtInputTextures[3].Sample(gssWrap, texCoord);
    float depth = objInfo.g;
    float brightness = dot(cColor.rgb, float3(0.299, 0.587, 0.114));
    float blurStrength = 0.2;
    if (brightness > 0.7f && depth >= 0.01f)
    {
        cColor = GaussianBlur(texCoord, blurStrength);
    }
    else if (depth >= 0.00000001 && depth <= 0.01)
    {
        blurStrength = 0.3f;
        cColor = GaussianBlur(texCoord, blurStrength);
    }

    return (cColor);
}