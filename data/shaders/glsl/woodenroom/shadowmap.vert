#version 450

layout (location = 0) in vec3 inPos;
layout (location = 4) in vec4 inJoint0;
layout (location = 5) in vec4 inWeight0;

layout (set = 0, binding = 1) uniform DirectionalLight {
	mat4 localToWorld;
    mat4 MVP;
	vec4 dirAndPower; // direction: xyz, power: w
	vec4 color; // color.w: reserved
} dirLight;

#define MAX_NUM_JOINTS 64
layout(set = 1, binding = 0) uniform UBONode
{
	mat4 matrix;
	mat4 jointMatrix[MAX_NUM_JOINTS];
	int jointCount;
}
node;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
    vec4 locPos;
    if (node.jointCount > 0) {
        // Mesh is skinned
        mat4 skinMat =
            inWeight0.x * node.jointMatrix[int(inJoint0.x)] + inWeight0.y * node.jointMatrix[int(inJoint0.y)] +
            inWeight0.z * node.jointMatrix[int(inJoint0.z)] + inWeight0.w * node.jointMatrix[int(inJoint0.w)];
        locPos = skinMat * vec4(inPos, 1.0);
    } else {
        locPos = node.matrix * vec4(inPos, 1.0);
    }

    vec3 outWorldPos = locPos.xyz / locPos.w;
    gl_Position = dirLight.MVP * vec4(outWorldPos, 1.0);
}