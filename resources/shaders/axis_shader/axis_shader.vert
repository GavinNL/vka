#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//========================================================================
// Input vertex attributes
//========================================================================


//========================================================================
// Output vertex attributes. Sent to the next stage in the shader
//========================================================================
layout(location = 0) out vec4 f_Color;

//========================================================================
// Per frame uniform data.
//========================================================================

//========================================================================
// Dynamic uniform buffer
//   used to store per object data which changes every frame
//========================================================================


//========================================================================
layout(push_constant) uniform PushConsts {
        mat4 mvp;
} pushConsts;


out gl_PerVertex 
{
    vec4 gl_Position;
};



void main() 
{
    vec3 x[4] = { vec3(0,0,0), vec3(1,0,0), vec3(0,1,0), vec3(0,0,1) };
    vec4 c[3] = { vec4(1,0,0,1), vec4(0,1,0,1), vec4(0,0,1,1) };


    gl_Position = pushConsts.mvp * vec4( x[ gl_VertexIndex%2==0?0: ( gl_VertexIndex/2+1) ], 1.0);

    f_Color     =  vec4(1,1,1,1);
    f_Color     =  c[ gl_VertexIndex/2 ];

}
