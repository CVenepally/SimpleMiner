#define MAX_LIGHTS 64
#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2

Texture2D       diffuseTexture : register(t0);
SamplerState    diffuseSampler : register(s0);

Texture2D       normalTexture  : register(t1);
SamplerState    normalSampler  : register(s1);

Texture2D       sgeTexture : register(t2);
SamplerState    sgeSampler : register(s2);

//------------------------------------------------------------------------------------------------
struct VertexInput
{
    float3 vi_position          : POSITION;
    float4 vi_vertexColor       : COLOR;
    float2 vi_texCoords         : TEXCOORD;
    float3 vi_vertexTangent     : TANGENT;
    float3 vi_vertexBitangent   : BITANGENT;
    float3 vi_vertexNormal      : NORMAL;
};

//------------------------------------------------------------------------------------------------
struct PixelInput
{
    float4 pi_position          : SV_POSITION;
    float3 pi_worldPosition     : POSITION;
    float4 pi_vertexColor        : COLOR;
    float2 pi_texCoords         : TEXCOORD;
    float3 pi_vertexTangent     : TANGENT0;
    float3 pi_vertexBitangent   : BITANGENT0;
    float3 pi_vertexNormal      : NORMAL0;
    float3 pi_worldTangent      : TANGENT1;
    float3 pi_worldBitangent    : BITANGENT1;
    float3 pi_worldNormal       : NORMAL1;
};

//------------------------------------------------------------------------------------------------
struct Light
{
    int     lightType;
    float3  position;
	//--------------------------------------16 bytes
	
    float3  direction;
    float   padding1;
    //--------------------------------------16 bytes
	
    float4  color;
	//--------------------------------------16 bytes	

    float   innerRadius;
    float   outerRadius;
    float   innerDot;
    float   outerDot;
};

//------------------------------------------------------------------------------------------------
cbuffer DebugConstants : register(b1)
{
    float   debugTime;
    int     debugInt;
    float   debugFloat;
    float   padding0;
}

//------------------------------------------------------------------------------------------------
cbuffer CameraConstants : register(b2)
{
    float4x4 worldToCameraTransform;
    float4x4 cameraToRenderTransform;
    float4x4 renderToClipTransform;
}

//------------------------------------------------------------------------------------------------
cbuffer ModelConstants : register(b3)
{
    float4x4    modelToWorldTransform;
    float4      modelColor;
    int         specialInt;
    float3      padding3;
}

//------------------------------------------------------------------------------------------------
cbuffer LightConstants : register(b4)
{
    Light   directionalLight; // 64 bytes
    Light   allLights[MAX_LIGHTS]; // 64 bytes
	
    float3  cameraPosition;
    int     numLights;
	//----------------------16
	
    float   ambientIntensity;
    float3  padding4;
	//-----------------------16	
}

//------------------------------------------------------------------------------------------------
PixelInput VertexMain(VertexInput vertexInput)
{
    float4 modelPosition    = float4(vertexInput.vi_position, 1);
    float4 worldPosition    = mul(modelToWorldTransform, modelPosition);
    float4 cameraPosition   = mul(worldToCameraTransform, worldPosition);
    float4 renderPosition   = mul(cameraToRenderTransform, cameraPosition);
    float4 clipPosition     = mul(renderToClipTransform, renderPosition);
    float4 worldTangent     = mul(modelToWorldTransform, float4(vertexInput.vi_vertexTangent,   0.f));
    float4 worldBitangent   = mul(modelToWorldTransform, float4(vertexInput.vi_vertexBitangent, 0.f));
    float4 worldNormal      = mul(modelToWorldTransform, float4(vertexInput.vi_vertexNormal,    0.f));
    
    PixelInput pixelIn;
    pixelIn.pi_position         = clipPosition;
    pixelIn.pi_worldPosition    = worldPosition.xyz;
    pixelIn.pi_texCoords        = vertexInput.vi_texCoords;
    pixelIn.pi_vertexColor      = vertexInput.vi_vertexColor;
    pixelIn.pi_vertexTangent    = vertexInput.vi_vertexTangent;
    pixelIn.pi_vertexBitangent  = vertexInput.vi_vertexBitangent;
    pixelIn.pi_vertexNormal     = vertexInput.vi_vertexNormal;
    pixelIn.pi_worldTangent     = worldTangent.xyz;
    pixelIn.pi_worldBitangent   = worldBitangent.xyz;
    pixelIn.pi_worldNormal      = worldNormal.xyz;
    
    return pixelIn;
}

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
float4 PixelMain(PixelInput pixelIn) : SV_Target0
{    
    float4 diffuseTexel     = diffuseTexture.Sample(diffuseSampler, pixelIn.pi_texCoords);
    float4 normalTexel      = normalTexture.Sample(normalSampler, pixelIn.pi_texCoords);
    float4 sgeTexel         = sgeTexture.Sample(normalSampler, pixelIn.pi_texCoords);
    float4 diffuseColor     = diffuseTexel * pixelIn.pi_vertexColor * modelColor;
    
    float specularity   = sgeTexel.r;
    float gloss         = sgeTexel.g;
    float emit          = sgeTexel.b;

    //--------------------------------------------------------------------------------------------------
    // Pixel Normal Calculations
    //--------------------------------------------------------------------------------------------------
    float3 worldTangent     = normalize(pixelIn.pi_worldTangent);
    float3 worldBitangent   = normalize(pixelIn.pi_worldBitangent);
    float3 worldNormal      = normalize(pixelIn.pi_worldNormal);

    float3x3 tbnToWorldTransform = float3x3(worldTangent, worldBitangent, worldNormal);
    
    float3 pixelNormalTBNSpace      = normalize(DecodeRGBtoXYZ(normalTexel.rgb));
    float3 pixelNormalWorldSpace    = mul(pixelNormalTBNSpace, tbnToWorldTransform);
    
    //--------------------------------------------------------------------------------------------------
    // Lighting
    //--------------------------------------------------------------------------------------------------  
    float3  finalDiffuseLighting    = float3(0.f, 0.f, 0.f);
    float3  finalSpecularLighting   = float3(0.f, 0.f, 0.f);
    float3  pixelToCamera           = normalize(cameraPosition - pixelIn.pi_worldPosition);
    float   specularExponent        = RangeMap(gloss, 0.f, 1.f, 1.f, 32.f);
    
    //--------------------------------------------------------------------------------------------------
    // Directional Lighting / Sun Light
    //--------------------------------------------------------------------------------------------------  
    float3 sunToPixel = directionalLight.direction;
    float3 pixelToSun = -sunToPixel;
 
    float sunDiffuseLightDot    = dot(pixelToSun, pixelNormalWorldSpace);    
    float sunLightStrength      = saturate(RangeMap(sunDiffuseLightDot, -ambientIntensity, 1.f, 0.f, 1.f));
    
    float3 sunDiffuse       = sunLightStrength * directionalLight.color.rgb;
    finalDiffuseLighting    += sunDiffuse;
    
    float3  sunIdealReflectionDirection = normalize(pixelToSun + pixelToCamera);
    float   sunSpecularDot              = saturate(dot(sunIdealReflectionDirection, pixelNormalWorldSpace));
    float   sunSpecularStrength         = gloss * directionalLight.color.a * pow(sunSpecularDot, specularExponent);
    float3  sunSpecularLight            = sunSpecularStrength * directionalLight.color.rgb;
    finalSpecularLighting += sunSpecularLight;
    
        
    //--------------------------------------------------------------------------------------------------
    // Emissive
    //--------------------------------------------------------------------------------------------------  
    float3 emissiveLight = diffuseTexel.rgb * emit;
    
    //--------------------------------------------------------------------------------------------------
    // Final Color
    //--------------------------------------------------------------------------------------------------  
    float3 finalRGB     = (saturate(finalDiffuseLighting) * diffuseColor.rgb) + (finalSpecularLighting * specularity) + emissiveLight;
    float4 finalColor   = float4(finalRGB, diffuseColor.a);
       
    // debug modes
    if(debugInt == 1)
    {
        finalColor = diffuseTexel;
    }
    else if(debugInt == 2)
    {
        finalColor = pixelIn.pi_vertexColor;
    }
    else if (debugInt == 3)
    {
        finalColor = modelColor;
    }
    else if(debugInt == 4)
    {
        finalColor.rgb = EncodeXYZtoRGB(pixelIn.pi_vertexNormal);
    }
    else if (debugInt == 5)
    {
        finalColor.rgb = EncodeXYZtoRGB(pixelIn.pi_vertexTangent);
    }
    else if (debugInt == 6)
    {
        finalColor.rgb = EncodeXYZtoRGB(pixelIn.pi_vertexBitangent);
    }
    else if (debugInt == 7)
    {
        finalColor.rgb = EncodeXYZtoRGB(worldNormal);
    }
    else if (debugInt == 8)
    {
        finalColor.rgb = EncodeXYZtoRGB(worldTangent);
    }
    else if (debugInt == 9)
    {
        finalColor.rgb = EncodeXYZtoRGB(worldBitangent);
    }
    else if(debugInt == 10)
    {
        finalColor.rgb = float3(pixelIn.pi_texCoords, 0.f);
    }
    else if(debugInt == 11)
    {
        finalColor.rgb = normalTexel.rgb;
    }
    else if (debugInt == 12)
    {
        finalColor.rgb = sgeTexel.rgb;
    }
    else if (debugInt == 13)
    {
        finalColor.rgb = finalSpecularLighting.rgb;
    }
    else if (debugInt == 14)
    {
        finalColor.rgb = finalDiffuseLighting.rgb;
    }
    else if (debugInt == 15)
    {
        finalColor.rgb = emissiveLight.rgb;
    }
    
    clip(finalColor.a - 0.01);
    
    return finalColor;
}