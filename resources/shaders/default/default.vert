#version 450
 
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set=1, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    int  use_uniform;
} cameraData;


layout(set = 2, binding = 0) uniform DynamicUniformBufferObject {
    mat4 model;
    int  index;
    int  miplevel;
} modelData;

layout(push_constant) uniform PushConsts {
        mat4 model;
        int  index;
        int  miplevel;
} pushConsts;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;


layout(location = 1) out vec4 fragTexCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{

    if( cameraData.use_uniform==1)
    {
        gl_Position = cameraData.proj * cameraData.view * modelData.model  * vec4(inPosition, 1.0);
        fragTexCoord = vec4(inTexCoord.xy, modelData.index, modelData.miplevel);
    }
    else
    {
        gl_Position = cameraData.proj * cameraData.view * pushConsts.model * vec4(inPosition, 1.0);
        fragTexCoord = vec4(inTexCoord.xy, pushConsts.index, pushConsts.miplevel);
    }



}
