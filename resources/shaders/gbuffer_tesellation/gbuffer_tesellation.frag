#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set=0, binding = 0) uniform sampler2DArray texSampler;

layout (location = 0) in vec3 f_Position;
layout (location = 1) in vec3 f_UV;
layout (location = 2) in vec3 f_Normal;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;

void main()
{
    outPosition = vec4(f_Position,1.0);

    outNormal = vec4(f_Normal,1.0);
    outAlbedo = texture(texSampler, f_UV).rgba;
}
