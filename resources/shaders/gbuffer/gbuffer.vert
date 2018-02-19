#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set=1, binding = 0) uniform UniformBufferObject {
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


layout (location = 0) out vec3 f_Normal;
layout (location = 1) out vec3 f_UV;
layout (location = 2) out vec3 f_WorldPos;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    gl_Position = cameraData.proj * cameraData.view * pushConsts.model * vec4(inPosition, 1.0);
    f_WorldPos  = gl_Position.xyz;
    f_Normal    = (pushConsts.model * vec4(inNormal, 1.0)).xyz;
    f_UV        = vec3(inTexCoord.xy, pushConsts.index );


}
