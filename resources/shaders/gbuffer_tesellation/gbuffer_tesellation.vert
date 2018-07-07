#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set=0, binding = 1) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} cameraData;


layout(push_constant) uniform PushConsts {
        mat4 model;
        int  index;
} pushConsts;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;


layout (location = 0) out vec3 tc_Position;
layout (location = 1) out vec3 tc_UV;
layout (location = 2) out vec3 tc_Normal;

out gl_PerVertex {
    vec4 gl_Position;
};


void main()
{
    tc_Position  = inPosition;
    tc_UV        = vec3(inTexCoord.xy, pushConsts.index );
    tc_Normal    = inNormal;

    gl_Position = vec4( tc_Position,  1.0);
}
