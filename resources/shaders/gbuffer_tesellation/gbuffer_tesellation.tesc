#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (vertices = 3) out;

layout (location = 0) in vec3 tc_Position[];
layout (location = 1) in vec3 tc_UV[];
layout (location = 2) in vec3 tc_Normal[];

layout (location = 0) out vec3 te_Position[3];
layout (location = 1) out vec3 te_UV[3];
layout (location = 2) out vec3 te_Normal[3];

layout(set=0, binding = 1) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} cameraData;

layout(push_constant) uniform PushConsts {
        mat4 model;
        int  index;
} pushConsts;

float screenSpaceTessFactor(vec4 p0, vec4 p1)
{
        float tessellatedEdgeSize = 60;
        float tessellationFactor = 0.3;

        // Calculate edge mid point
        vec4 midPoint = 0.5 * (p0 + p1);

        // Sphere radius as distance between the control points
        float radius = distance(p0, p1) / 2.0;

        // View space
        vec4 v0 = cameraData.view * pushConsts.model  * midPoint;

        // Project into clip space
        vec4 clip0 = (cameraData.proj * (v0 - vec4(radius, vec3(0.0))));
        vec4 clip1 = (cameraData.proj * (v0 + vec4(radius, vec3(0.0))));

        // Get normalized device coordinates
        clip0 /= clip0.w;
        clip1 /= clip1.w;

        // Convert to viewport coordinates
        clip0.xy *= vec2(1920,1200);//ubo.viewportDim;
        clip1.xy *= vec2(1920,1200);//ubo.viewportDim;

        // Return the tessellation factor based on the screen size
        // given by the distance of the two edge control points in screen space
        // and a reference (min.) tessellation size for the edge set by the application
        float dist = distance(clip0,clip1);
        return clamp( dist / tessellatedEdgeSize * tessellationFactor, 1.0, 64.0);
}

// return a tessellation factor based on how far the
// edge midpoint is away from the camera.
float viewSpaceTesselation(vec4 p0, vec4 p1)
{
        float max_range           = 20;
        float tessellatedEdgeSize = 3;
        float max_tessellation    = 8;

        // Calculate edge mid point
        vec4 midPoint = 0.5 * (p0 + p1);

        // Transform the midpoint into view space
        vec4 v0 = cameraData.view * pushConsts.model  * midPoint;

        // tessellate edges wich are between 0 and 20 units from teh camera
        float dist =  distance( v0, vec4(0,0,0,1) );

        float s  = dist / tessellatedEdgeSize;

        float f  = max_tessellation - s;

        return clamp(f,1,max_tessellation);
}


void main(void)
{
    if (gl_InvocationID == 0)
    {
        vec4 p0 = gl_in[0].gl_Position;
        vec4 p1 = gl_in[1].gl_Position;
        vec4 p2 = gl_in[2].gl_Position;

        gl_TessLevelOuter[2] = screenSpaceTessFactor(p0, p1 );
        gl_TessLevelOuter[0] = screenSpaceTessFactor(p1, p2 );
        gl_TessLevelOuter[1] = screenSpaceTessFactor(p0, p2 );

        gl_TessLevelInner[0] =  max(gl_TessLevelOuter[0], max(gl_TessLevelOuter[1], gl_TessLevelOuter[2]) );

    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    te_Position[gl_InvocationID]        = tc_Position[gl_InvocationID];
    te_UV[gl_InvocationID]              = tc_UV[gl_InvocationID];
    te_Normal[gl_InvocationID]          = tc_Normal[gl_InvocationID];
}

