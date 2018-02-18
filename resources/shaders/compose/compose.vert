#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//========================================================================
// Input vertex attributes
//========================================================================


//========================================================================
// Output vertex attributes. Sent to the next stage in the shader
//========================================================================
layout(location = 0) out vec4 f_Position;
layout(location = 1) out vec4 f_UV;
layout(location = 2) out vec4 f_Color;

//========================================================================
// Per frame uniform data.
//========================================================================

//========================================================================
// Dynamic uniform buffer
//   used to store per object data which changes every frame
//========================================================================


//========================================================================
layout(push_constant) uniform PushConsts {
        int layer;
} pushConsts;


out gl_PerVertex 
{
    vec4 gl_Position;
};



void main() 
{
    vec2 x[6] = {
     vec2(-1   ,-1.0 )  , vec2(-1.0, 1.0 )   , vec2( 1.0,  1.0 )  ,
     vec2(-1   ,-1.0 )  , vec2( 1.0,  1.0 )  , vec2( 1.0, -1.0 )
    };

    vec2 U[6] = {
     vec2(0    , 0.0 )  , vec2( 0.0, 1.0 )   , vec2( 1.0,  1.0 )  ,
     vec2(0    , 0.0 )  , vec2( 1.0,  1.0 )  , vec2( 1.0,  0.0 )
    };
    //vec2 x[6] = { vec2(-1,1), vec2(-1,-1), vec2( 1,-1), vec2(1,-1), vec2( 1, 1), vec2(-1,1)  };
    vec3 u[6] = { vec3( 1, 1, 0),
                  vec3( 0, 1, 1),
                  vec3( 1, 0, 1),

                  vec3( 1,  0, 0),
                  vec3( 0,  1, 0),
                  vec3( 0,  0, 1) };

    gl_Position = vec4( x[ gl_VertexIndex ], 0,  1.0);
    f_Position  = gl_Position;
    f_Color     = vec4( u[ gl_VertexIndex ], 1.0);
    f_UV        = vec4( U[gl_VertexIndex], 0 ,0);
}
