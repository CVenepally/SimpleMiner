cbuffer CameraConstants : register(b2)
{
	float4x4 WorldToCameraTransform;
	float4x4 CameraToRenderTransform;
	float4x4 RenderToClipTransform;
};

//------------------------------------------------------------------------------------------------
cbuffer ModelConstants : register(b3)
{
	float4x4	ModelToWorldTransform;
	float4		modelColor;
	int			specialInt;
	float3		Padding3;
};

//------------------------------------------------------------------------------------------------
cbuffer WorldConstants : register(b0)
{
    float4 m_playerPosition;
    float4 m_skyColor;
    float4 m_indoorLightColor;
    float4 m_outdoorLightColor;
    float2 m_distanceRange;
    float2 m_padding0;
}

//------------------------------------------------------------------------------------------------
struct vs_input_t
{
	float3 modelSpacePosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

//------------------------------------------------------------------------------------------------
struct v2p_t
{
	float4 clipSpacePosition : SV_Position;
    float4 worldPixelPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

//------------------------------------------------------------------------------------------------
float3 EncodeXYZtoRGB(float3 xyzToEncode)
{
    return (xyzToEncode + 1.f) * 0.5f;
}

//------------------------------------------------------------------------------------------------
float3 DecodeRGBtoXYZ(float3 rgbToDecode)
{
    return (rgbToDecode * 2.f) - 1.f;
}

//------------------------------------------------------------------------------------------------
float RangeMapClamped(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
    float fraction = saturate((inValue - inStart) / (inEnd - inStart));
    float outValue = outStart + fraction * (outEnd - outStart);
    return outValue;
}

//------------------------------------------------------------------------------------------------
float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
    float fraction = (inValue - inStart) / (inEnd - inStart);
    float outValue = outStart + fraction * (outEnd - outStart);
    return outValue;
}

//------------------------------------------------------------------------------------------------
v2p_t VertexMain(vs_input_t input)
{
	
	float4 modelSpacePosition = float4(input.modelSpacePosition, 1);
 	float4 worldSpacePosition = mul(ModelToWorldTransform, modelSpacePosition);
 	float4 viewSpacePosition = mul(WorldToCameraTransform, worldSpacePosition);
 	float4 renderSpacePosition = mul(CameraToRenderTransform, viewSpacePosition);
	float4 clipSpacePosition = mul(RenderToClipTransform, renderSpacePosition);

	v2p_t v2p;
	v2p.clipSpacePosition = clipSpacePosition;
    v2p.worldPixelPosition = worldSpacePosition;
	v2p.color = input.color;
	v2p.uv = input.uv;
	return v2p;
}

//------------------------------------------------------------------------------------------------
Texture2D diffuseTexture : register(t0);
SamplerState diffuseSampler : register(s0);

float4 DiminishingAdd(float4 a, float4 b)
{
    return 1.f.xxxx - (1.f.xxxx - a) * (1.f.xxxx - b);
}

//------------------------------------------------------------------------------------------------
float4 PixelMain(v2p_t input) : SV_Target0
{
	float4 textureColor = diffuseTexture.Sample(diffuseSampler, input.uv);
	float4 vertexColor = input.color;
	
    float outLightInfluence = vertexColor.r;
    float inLightInfluence = vertexColor.g;
    float vertGreyScale = vertexColor.b;
	
    float4 diminishingA = outLightInfluence * m_outdoorLightColor;
    float4 diminishingB = inLightInfluence * m_indoorLightColor;
	
    float4 diffuseLight = DiminishingAdd(diminishingA, diminishingB);
	
    float4 pixelColor = textureColor * diffuseLight * vertGreyScale;
    
    pixelColor.a = 1.f;
    
    float pixelToPlayerDistance = length(m_playerPosition - input.worldPixelPosition);
	
    float fogFraction = RangeMapClamped(pixelToPlayerDistance, m_distanceRange.x, m_distanceRange.y, 0, 1);
	
    float4 finalColor = lerp(pixelColor, m_skyColor, fogFraction);
	
    clip(finalColor.a - 0.01f);
    return float4(finalColor);
}
