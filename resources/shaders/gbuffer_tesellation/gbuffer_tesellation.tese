#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (triangles) in;

layout(set=0, binding = 1) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} cameraData;


layout(push_constant) uniform PushConsts {
        mat4 model;
        int  index;
} pushConsts;


layout (location = 0) in vec3 te_Position[];
layout (location = 1) in vec3 te_UV[];
layout (location = 2) in vec3 te_Normal[];

layout (location = 0) out vec3 f_Position;
layout (location = 1) out vec3 f_UV;
layout (location = 2) out vec3 f_Normal;

void main(void)
{
    // model space position
    gl_Position = (gl_TessCoord.x * gl_in[0].gl_Position) +
                  (gl_TessCoord.y * gl_in[1].gl_Position) +
                  (gl_TessCoord.z * gl_in[2].gl_Position);


        // model space position
        f_UV       = gl_TessCoord.x*te_UV[0]       + gl_TessCoord.y*te_UV[1]       + gl_TessCoord.z*te_UV[2];
        f_Normal   = gl_TessCoord.x*te_Normal[0]   + gl_TessCoord.y*te_Normal[1]   + gl_TessCoord.z*te_Normal[2];

        // World space Position
        f_Position  = (pushConsts.model * gl_Position ).xyz;
        f_Normal    = normalize(pushConsts.model * vec4(f_Normal, 1.0)).xyz;

        gl_Position = cameraData.proj * cameraData.view * pushConsts.model * gl_Position;


}
