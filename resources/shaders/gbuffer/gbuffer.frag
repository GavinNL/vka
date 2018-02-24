#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set=0, binding = 0) uniform sampler2DArray texSampler;



layout (location = 0) in vec3 f_Normal;
layout (location = 1) in vec3 f_UV;
//layout (location = 2) in vec3 f_Color;
layout (location = 2) in vec3 f_WorldPos;
//layout (location = 4) in vec3 f_Tangent;


layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;

void main()
{
    outPosition = vec4(f_WorldPos,1.0);

    /*
    // Calculate normal in tangent space
    vec3 N = normalize(f_Normal);
    N.y = -N.y;

    vec3 T = normalize(inTangent);
    vec3 B = cross(N, T);

    mat3 TBN = mat3(T, B, N);

    vec3 tnorm = TBN * normalize(texture(samplerNormalMap, inUV).xyz * 2.0 - vec3(1.0));
    */

    outNormal = vec4(f_Normal,1.0);
    outAlbedo = texture(texSampler, f_UV).rgba;
}
