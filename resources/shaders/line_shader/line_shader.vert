#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable



//========================================================================
// Input vertex attributes
//========================================================================
layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec4 v_Color;


//========================================================================
// Output vertex attributes. Sent to the next stage in the shader
//========================================================================
layout(location = 0) out vec4 f_Color;


//========================================================================
// Per frame uniform data.
//========================================================================
layout(set = 0, binding = 0) uniform UniformBufferObject 
{
    mat4 view;
    mat4 proj;

} u_CameraData;



//========================================================================
// Dynamic uniform buffer
//   used to store per object data which changes every frame
//========================================================================
layout(set = 1, binding = 0) uniform DynamicUniformBufferObject 
{
    mat4 model;
} u_ModelData;
//========================================================================



out gl_PerVertex 
{
    vec4 gl_Position;
};



void main() 
{
    gl_Position = u_CameraData.proj * u_CameraData.view * u_ModelData.model * vec4(v_Position, 1.0);

    f_Color     =  v_Color;

}
