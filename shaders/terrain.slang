import toolbox;

// Input and output structures
struct VertexInput {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
    float4 tangent  : TANGENT;
};

struct VertexOutput {
    float2 uv;
    float3 worldNormal;
    float4 position : SV_Position;
    float4 tangent;
};

struct PixelOutput {
    float4 color : SV_Target;
};

struct TerrainData{
    float4x4 modelViewProj;
    float4 lightDirection;
    float4 lightColor;
    float4 ambientLightColor;
};


[vk::push_constant] TerrainData terrainData;


// Vertex Shader
[shader("vertex")]
VertexOutput vertexMain(VertexInput input) {
    VertexOutput output;

    // This is so that slang compiler does not optimize out the tangent as it is not really used here, but the engine passes it anyway
    // This way we do not get the performance warning
    output.tangent = input.tangent;

    // Transform the position to clip space
    float4 worldPosition = float4(input.position, 1.0);
    output.position = mul(terrainData.modelViewProj, worldPosition);

    // Pass normal in world space (or object space if no model matrix is applied)
    output.worldNormal = normalize(input.normal);

    // Pass through UVs
    output.uv = input.uv;

    return output;
}

[shader("pixel")]
PixelOutput pixelMain(VertexOutput input, float4 fragCoord : SV_Position) {
    PixelOutput output;

    // Lambert shading
    float3 lightDir = normalize(terrainData.lightDirection.xyz);
    float NdotL = max(dot(input.worldNormal, lightDir), 0.0);

    float3 lightColor = terrainData.lightColor.rgb;
    float3 ambientLightColor = terrainData.ambientLightColor.rgb;
    float3 albedo = float3(0.08); 

    float3 diffuse = albedo * lightColor * NdotL;
    float3 ambient = albedo * ambientLightColor;

    output.color = float4(saturate(diffuse + ambient), 1.0);

    return output;
}