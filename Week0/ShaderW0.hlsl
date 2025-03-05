struct VS_INPUT
{
    float4 position : POSITION; 
    float4 color : COLOR;       
};

struct PS_INPUT
{
    float4 position : SV_POSITION; 
    float4 color : COLOR;          
};

cbuffer constants : register(b0)
{
    float3 Offset;
    float Radius;
    float Rotation;
    float Pad;
}

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    float3 scaledPos = input.position.xyz * Radius;

    float cosTheta = cos(Rotation);
    float sinTheta = sin(Rotation);

    float3 rotatedPos;
    rotatedPos.x = scaledPos.x * cosTheta - scaledPos.y * sinTheta;
    rotatedPos.y = scaledPos.x * sinTheta + scaledPos.y * cosTheta;
    rotatedPos.z = scaledPos.z;

    float3 translatedPos = rotatedPos + Offset;

    output.position = float4(translatedPos, input.position.w);

    output.color = input.color;

    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    return input.color;
}
