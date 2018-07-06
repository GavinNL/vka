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

    //vec2 x[6] = {
    // vec2(-1   ,-1.0 )  , vec2(-1.0, 1.0 )   , vec2( 1.0,  1.0 )  ,
    // vec2(-1   ,-1.0 )  , vec2( 1.0,  1.0 )  , vec2( 1.0, -1.0 )
    //};
    float T = 2*3.14159 / 3;
    //vec2 x[3] = {
    //    vec2( cos(T*0), sin(T*0) ),
    //    vec2( cos(T*1), sin(T*1) ),
    //    vec2( cos(T*2), sin(T*2) )
    //};
    vec2 x[3] = {
        vec2( -1, -1 ),
        vec2( -1,  1 ),
        vec2(  1,  1 )
    };



    tc_Position  = inPosition;//vec3( inPosition, 1.0).xyz;
    //tc_Position  = vec3(x[ gl_VertexIndex],0);

    tc_UV        = vec3(inTexCoord.xy, pushConsts.index );
    tc_Normal    = inNormal;
    //tc_Normal    = normalize(pushConsts.model * vec4(inNormal, 1.0)).xyz;

    gl_Position = vec4( tc_Position,  1.0);
}
