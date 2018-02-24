#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set=0, binding = 0) uniform sampler2DArray texSampler;

layout(location = 1) in vec4 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    if( fragTexCoord.w >=0 )
    {
        outColor = textureLod(texSampler, fragTexCoord.rgb , fragTexCoord.w   ).rgba;
    } else {
        outColor = texture(texSampler, fragTexCoord.rgb   ).rgba;
    }
}
