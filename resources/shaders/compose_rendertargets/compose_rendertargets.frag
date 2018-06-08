#version 450
#extension GL_ARB_separate_shader_objects : enable


//========================================================================
// Input attributes.
//========================================================================
layout(location = 0) in vec4 f_Position;
layout(location = 1) in vec4 f_UV;
layout(location = 2) in vec4 f_Color;


//========================================================================
layout(location = 0) out vec4 out_Color;


//========================================================================
layout(set=0, binding = 0) uniform sampler2D texSampler0;
layout(set=0, binding = 1) uniform sampler2D texSampler1;
layout(set=0, binding = 2) uniform sampler2D texSampler2;
layout(set=0, binding = 3) uniform sampler2D texSampler3;

//========================================================================
// Push constants. This should be in all stages.
//========================================================================
layout(push_constant) uniform PushConsts {
        int layer;
} pushConsts;

void main() 
{    
    switch(pushConsts.layer)
    {
        case 0:
            out_Color = texture(texSampler0, f_UV.xy ).rgba;
            break;
        case 1:
            out_Color = texture(texSampler1, f_UV.xy ).rgba;
            break;
        case 2:
            out_Color = texture(texSampler2, f_UV.xy ).rgba;
            break;
    }

}
