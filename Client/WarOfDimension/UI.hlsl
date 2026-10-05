#include "Common.hlsl"

float4 SampleUiTexture(float2 uv)
{
    return gtxtAlbedoTexture.Sample(gssWrap, UiTextureCoordinates(uv, gUiUvTransform));
}

VS_TEXTURED_OUTPUT VSTextured(VS_TEXTURED_INPUT input)
{
    VS_TEXTURED_OUTPUT output;

    matrix world = matrix(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);

    output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), world), gmtxOrtho);
    output.uv = float2(input.uv.x * -1.0f, input.uv.y);

    return (output);
}

float4 PSTextured(VS_TEXTURED_OUTPUT input) : SV_TARGET
{
	/*float time = gCurrentTime;
	
	float2 newUV = WaveDistortion(input.uv, 10.0, 0.1, time);*/

    float4 cColor = SampleUiTexture(input.uv);
	
    switch (gTextureKind)
    {
        case 0:
        case 9:
            cColor.rgb *= gTextureValue;
            break;
        case 1:
            cColor.a = gTextureValue;
            break;
        case 2:
            cColor.a = ((input.uv.x * -1.0) >= (1.0 - gTextureValue)) ? cColor.a : 0.f;
            break;
        case 3:
            cColor.a = ((input.uv.x * -1.0) <= gTextureValue) ? cColor.a : 0.f;
            break;
        case 5:
            cColor = SampleUiTexture(float2((1.0f - input.uv.x * -1.0f) / 12.0 + gTextureUVx, input.uv.y / 4.0 + gTextureUVy));
            cColor.a = gTextureValue;
            break;
        case 6:
            cColor = SampleUiTexture(float2((1.0f - input.uv.x * -1.0f) / 10.0 + gTextureUVx, input.uv.y / 2.0 + gTextureUVy));
            cColor.a = gTextureValue;
            break;
        case 7:
            cColor = SampleUiTexture(float2((1.0f - input.uv.x * -1.0f) / 6.0f + gTextureUVx, input.uv.y));
            break;
        case 8:
            cColor = SampleUiTexture(float2((1.0f - input.uv.x * -1.0f) / 11.0f + gTextureUVx, input.uv.y));
            break;
        case 10:
            cColor = SampleUiTexture(float2((1.0f - input.uv.x * -1.0f) / 7.0 + gTextureUVx, input.uv.y / 3.0 + gTextureUVy));
            cColor.rgb *= gTextureValue;
            break;
        case 11:
            cColor = SampleUiTexture(float2((1.0f - input.uv.x * -1.0f) / 38.0 + gTextureUVx, input.uv.y / 40.0 + gTextureUVy));
            cColor.rgb *= gTextureValue;
            break;
        case 12:
		{
                float brightness = dot(cColor.rgb, float3(0.299, 0.587, 0.114));
                float twinkleFactor = 0.0;
                if (brightness > 0.8)
                {
                    twinkleFactor = Twinkle(input.position.xyz, 1.5f, 0.4f, true);
                }
                cColor.rgb += twinkleFactor * 1.5f;
                break;
            }
        case 13:
            cColor = SampleUiTexture(float2((1.0f - input.uv.x * -1.0f) / 3.0 + gTextureUVx, input.uv.y / 4.0 + gTextureUVy));
            cColor.rgb *= gTextureValue;
            break;
        case 14:
        {
                float twinkleFactor = Twinkle(input.position.xyz, 3.0f, 0.3f, true);
                cColor.rgb -= twinkleFactor;
            }
            break;
        case 15:
        //cColor = SampleUiTexture(float2((1.0f - input.uv.x * -1.0f) / 4.0 + gTextureUVx, input.uv.y));
            cColor = GaussianBlur(float2((1.0f - input.uv.x * -1.0f) / 4.0 + gTextureUVx, input.uv.y), 0.7f, gUiUvTransform);
            cColor.rgb *= 0.7;
            cColor.a -= 0.5f;
            break;
        case 16:
		{
                float brightness = dot(cColor.rgb, float3(0.299, 0.587, 0.114));
                float twinkleFactor = 0.0;
                if (brightness > 0.5)
                {
                    twinkleFactor = Twinkle(input.position.xyz, 1.5f, 0.2f, true);
                }
                cColor.rgb += twinkleFactor * 1.5f;
                break;
            }
        case 17:
        {
                float twinkleFactor = Twinkle(input.position.xyz, 1.5f, 0.3f, true);
                cColor.rgb -= twinkleFactor;
            }
            break;
        case 18:
        {
                cColor = SampleUiTexture(float2((1.0f - input.uv.x * -1.0f) / 12.0 + gTextureUVx, input.uv.y / 6.0 + gTextureUVy));
                cColor.rgb *= gTextureValue;
            }
            break;
        default:
            break;
    }

	//cColor = GaussianBlur(input.uv);
    return (cColor);
}