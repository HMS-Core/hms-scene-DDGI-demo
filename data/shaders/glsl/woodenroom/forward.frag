#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inWorldPos;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec4 inShadowCoord;
layout (location = 4) in mat3 inTBN;

layout(location = 0) out vec4 resultImage;

layout (set = 0, binding = 0) uniform UBO
{
    mat4 projection;
    mat4 view;
    vec3 cameraPos;
} ubo;

layout (set = 0, binding = 1) uniform DirectionalLight {
    mat4 localToWorld;
    mat4 MVP;
    vec4 dirAndPower; // direction: xyz, power: w
    vec4 color; // color.w: reserved
} dirLight;

layout (set = 0, binding = 2) uniform ShadingParameter {
    float gamma;
    int ddgiDownSizeScale;
    int ddgiTexWidth;
    int ddgiTexHeight;
    int ddgiEffectOn;
} shadingPara;

layout(push_constant) uniform Material
{
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    int baseColorTextureSet;
    int physicalDescriptorTextureSet;
    int normalTextureSet;
    int occlusionTextureSet;
    int emissiveTextureSet;
} material;

layout(set = 1, binding = 0) uniform sampler2D irradianceTex;
layout(set = 1, binding = 1) uniform sampler2D normalDepthTex;
layout(set = 1, binding = 2) uniform sampler2D shadowmapTex;

layout(set = 3, binding = 0) uniform sampler2D colorMap;
layout(set = 3, binding = 1) uniform sampler2D normalMap;
layout(set = 3, binding = 2) uniform sampler2D physicalDescriptorMap;
layout(set = 3, binding = 3) uniform sampler2D aoMap;
layout(set = 3, binding = 4) uniform sampler2D emissiveMap;

vec3 ACESFilm(vec3 x)
{
    float a = 2.5099999904632568359375;
    float b = 0.02999999932944774627685546875;
    float c = 2.4300000667572021484375;
    float d = 0.589999973773956298828125;
    float e = 0.14000000059604644775390625;
    return clamp((x * ((x * a) + vec3(b))) / ((x * ((x * c) + vec3(d))) + vec3(e)), vec3(0.0), vec3(1.0));
}

vec3 srgb_to_linear(vec3 c)
{
    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c));
}

const float PI = 3.141592653589793;
const float INV_PI = 0.31830988618;

vec3 Bilateral(ivec2 uv, vec3 normal)
{
    float reDepth = 1 / gl_FragCoord.w;
    vec3 irrad = vec3(0.0);
    float sumWeight = 0.0;
    float weight = 0.0;
    float deltaDepth = 0.0;
    float normalDistance = 0.0;
    vec4 downIrrad = texelFetch(irradianceTex, uv, 0);
    vec4 downNormal = texelFetch(normalDepthTex, uv, 0);
    deltaDepth = abs(reDepth - downNormal.w);
    normalDistance = dot(downNormal.xyz, normal);
    if ((normalDistance > 0.930000007152557373046875) && (deltaDepth < 0.05 * reDepth)) {
        return downIrrad.xyz;
    }
    weight = 0.5 * (1.0 / (9.9999997473787516355514526367188e-05 + deltaDepth)) * pow(normalDistance, 4.0);
    irrad += (downIrrad.xyz * weight);
    sumWeight += weight;
    downIrrad = texelFetch(irradianceTex, uv + ivec2(0, 1), 0);
    downNormal = texelFetch(normalDepthTex, uv + ivec2(0, 1), 0);
    deltaDepth = abs(reDepth - downNormal.w);
    normalDistance = dot(downNormal.xyz, normal);
    if ((normalDistance > 0.930000007152557373046875) && (deltaDepth < 0.05 * reDepth)) {
        return downIrrad.xyz;
    }
    weight = 0.25 * (1.0 / (9.9999997473787516355514526367188e-05 + deltaDepth)) * pow(normalDistance, 4.0);
    irrad += (downIrrad.xyz * weight);
    sumWeight += weight;
    downIrrad = texelFetch(irradianceTex, uv + ivec2(1, 0), 0);
    downNormal = texelFetch(normalDepthTex, uv + ivec2(1, 0), 0);
    deltaDepth = abs(reDepth - downNormal.w);
    normalDistance = dot(downNormal.xyz, normal);
    if ((normalDistance > 0.930000007152557373046875) && (deltaDepth < 0.05 * reDepth)) {
        return downIrrad.xyz;
    }
    weight = 0.25 * (1.0 / (9.9999997473787516355514526367188e-05 + deltaDepth)) * pow(normalDistance, 4.0);
    irrad += (downIrrad.xyz * weight);
    sumWeight += weight;
    downIrrad = texelFetch(irradianceTex, uv + ivec2(-1, 0), 0);
    downNormal = texelFetch(normalDepthTex, uv + ivec2(-1, 0), 0);
    deltaDepth = abs(reDepth - downNormal.w);
    normalDistance = dot(downNormal.xyz, normal);
    if ((normalDistance > 0.930000007152557373046875) && (deltaDepth < 0.05 * reDepth)) {
        return downIrrad.xyz;
    }
    weight = 0.25 * (1.0 / (9.9999997473787516355514526367188e-05 + deltaDepth)) * pow(normalDistance, 4.0);
    irrad += (downIrrad.xyz * weight);
    sumWeight += weight;
    downIrrad = texelFetch(irradianceTex, uv + ivec2(0, -1), 0);
    downNormal = texelFetch(normalDepthTex, uv + ivec2(0, -1), 0);
    deltaDepth = abs(reDepth - downNormal.w);
    normalDistance = dot(downNormal.xyz, normal);
    if ((normalDistance > 0.930000007152557373046875) && (deltaDepth < 0.05 * reDepth)) {
        return downIrrad.xyz;
    }
    weight = 0.25 * (1.0 / (9.9999997473787516355514526367188e-05 + deltaDepth)) * pow(normalDistance, 4.0);
    irrad += (downIrrad.xyz * weight);
    sumWeight += weight;
    return irrad / vec3(sumWeight + 9.9999997473787516355514526367188e-06);
}

float textureProj(vec4 shadowCoord, vec2 off)
{
    float shadow = 1.0;
    if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 )
    {
        float dist = texture(shadowmapTex, shadowCoord.st + off).r;
        if (shadowCoord.w > 0.0 && dist < (shadowCoord.z - 0.01)) {
            shadow = 0.0;
        }
    }
return shadow;
}

// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    vec3 camPos = ubo.cameraPos.xyz;
    vec3 V = normalize(camPos.xyz - inWorldPos);
    vec3 N = normalize(inNormal.xyz);

    if (material.normalTextureSet > -1) {
        N = normalize(inTBN * normalize(2 * srgb_to_linear(texture(normalMap, inUV).xyz)));
    }

    vec3 baseColor = srgb_to_linear(material.baseColorFactor.xyz);
    if (material.baseColorTextureSet > -1) {
        baseColor = texture(colorMap, inUV).xyz;
        baseColor.xyz = srgb_to_linear(baseColor.xyz);
    }

    float roughness = material.roughnessFactor;
    if (material.physicalDescriptorTextureSet > -1) {
        roughness *= texture(physicalDescriptorMap, inUV).y;
    }

    float metalness = material.metallicFactor;
    if (material.physicalDescriptorTextureSet > -1) {
        metalness *= texture(physicalDescriptorMap, inUV).z;
    }

    vec3 irradiancess = vec3(0.0);
    ivec2 texUV = ivec2(gl_FragCoord.xy);
    texUV.y = shadingPara.ddgiTexHeight - texUV.y;
    if (shadingPara.ddgiDownSizeScale == 1) {
        irradiancess = texelFetch(irradianceTex, texUV, 0).xyz;
    } else {
        ivec2 inDirectUV = ivec2(vec2(texUV) / vec2(shadingPara.ddgiDownSizeScale));
        irradiancess = Bilateral(inDirectUV, N);
    }

    float shadow = textureProj(inShadowCoord / inShadowCoord.w, vec2(0.0));

    vec3 L = normalize(-dirLight.dirAndPower.xyz);
    vec3 H = normalize(V + L);
    float NoV = clamp(dot(N, V), 0.0, 1.0);
    float NoL = clamp(dot(N, L), 0.0, 1.0);
    float NoH = clamp(dot(N, H), 0.0, 1.0);
    float VoH = clamp(dot(V, H), 0.0, 1.0);

    vec3 directionLightRes = vec3(0.0);
    vec3 directRes = vec3(0.0);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, baseColor.xyz, metalness);
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(VoH, F0);

    vec3 specular = (metalness * baseColor.xyz) * (D * F * G) / (4.0 * NoL * NoV + 0.001);
    vec3 diffuse = (1.0 - metalness) * baseColor.xyz * (vec3(1.0) - F) * INV_PI;
    directRes = (diffuse + specular) * dirLight.dirAndPower.w * NoL;
    directRes *= shadow;
    vec3 indirectDiffuse = vec3(0.0);
    if (shadingPara.ddgiEffectOn == 1) {
        indirectDiffuse = baseColor * INV_PI * irradiancess;
    }

    vec3 result_t = directRes + indirectDiffuse;
    vec3 param_1 = result_t;
    result_t = ACESFilm(param_1);
    result_t = pow(result_t, vec3(1 / 2.2));
    resultImage = vec4(result_t, 1.0);
}