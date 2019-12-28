cbuffer LightCBuf
{
    float3 lightPos;
    float3 ambient;
    float3 diffuseColor;
    float diffuseIntensity;
    float attConst;
    float attLin;
    float attQuad;
};
cbuffer ObjectBuf
{
    float specularIntensity;
    float specularPower;
    bool normalEnabled;
    float padding;
};

Texture2D tex;
Texture2D nmap : register(t2);
SamplerState smpl;


float4 main(float3 worldPos : Position, float3 n : Normal, float3 tan : Tangent, float3 bitan : Bitangent, float2 tc : TEXCOORD) : SV_Target
{
    if (normalEnabled)
    {
        const float3x3 tanToView = float3x3(
            normalize(tan),
            normalize(bitan),
            normalize(n)
        );
        
        const float3 normalSample = nmap.Sample(smpl, tc).xyz;
        n.x = normalSample.x * 2.0f - 1.0f;
        n.y = -normalSample.y * 2.0f + 1.0f;
        n.z = normalSample.z;
        n = mul(n, (float3x3) tanToView);
    }
	// fragment to light vector data
    const float3 vToL = lightPos - worldPos;
    const float distToL = length(vToL);
    const float3 dirToL = vToL / distToL;
	// diffuse attenuation
    const float att = 1.0f / (attConst + attLin * distToL + attQuad * (distToL * distToL));
	// diffuse intensity
    const float3 diffuse = diffuseColor * diffuseIntensity * att * max(0.0f, dot(dirToL, n));
    // reflected light vector
    const float3 w = n * dot(vToL, n);
    const float3 r = w * 2.0f - vToL;
    // calculate specular intensity
    const float3 specular = att * (diffuseColor * diffuseIntensity) * specularIntensity * pow(max(0.0f, dot(normalize(-r), normalize(worldPos))), specularPower);
	// final color
    return float4(saturate((diffuse + ambient) * tex.Sample(smpl, tc).rgb + specular), 1.0f);
}