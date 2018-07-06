#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (vertices = 3) out;

layout (location = 0) in vec3 tc_Normal[];
layout (location = 1) in vec3 tc_UV[];
layout (location = 2) in vec3 tc_Position[];

layout (location = 0) out vec3 te_Position[];
layout (location = 1) out vec3 te_UV[];
layout (location = 2) out vec3 te_Normal[];

void main(void)
{
    if (gl_InvocationID == 0)
    {
        gl_TessLevelInner[0] = 1.0;
        gl_TessLevelOuter[0] = 1.0;
        gl_TessLevelOuter[1] = 1.0;
        gl_TessLevelOuter[2] = 1.0;
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    te_Position[gl_InvocationID]        = tc_Position[gl_InvocationID];
    te_Normal[gl_InvocationID]          = tc_Normal[gl_InvocationID];
    te_UV[gl_InvocationID]              = tc_UV[gl_InvocationID];
}
