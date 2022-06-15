#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 6) in vec4 inTangent;

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

#define MAX_NUM_JOINTS 64
layout(set = 2, binding = 0) uniform UBONode
{
	mat4 matrix;
	mat4 jointMatrix[MAX_NUM_JOINTS];
	int jointCount;
}
node;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outWorldPos;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec4 outShadowCoord;
layout (location = 4) out mat3 TBN;

out gl_PerVertex
{
	vec4 gl_Position;
};

const mat4 biasMat = mat4( 
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.0, 1.0 );

void main() 
{
    vec4 locPos;

    locPos = node.matrix * vec4(inPos, 1.0);
	mat3 normalMatrix = transpose(inverse(mat3(node.matrix)));
    outNormal = normalize(normalMatrix * inNormal);
	vec3 T = normalize(normalMatrix * inTangent.xyz);
    vec3 B = normalize(cross(outNormal, T) * inTangent.w);
    TBN = mat3(T, B, outNormal);

    outWorldPos = locPos.xyz / locPos.w;
    outUV = inUV;
//	outShadowCoord = dirLight.MVP * locPos;
//	outShadowCoord.xyz = outShadowCoord.xyz / outShadowCoord.w;
//    outShadowCoord.xy = outShadowCoord.xy * 0.5 + 0.5;
    outShadowCoord = (biasMat * dirLight.MVP * node.matrix) * vec4(inPos, 1.0);	
    gl_Position = ubo.projection * ubo.view * vec4(outWorldPos, 1.0);
}