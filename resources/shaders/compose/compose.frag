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
layout(set=0, binding = 0) uniform sampler2D texSampler0; // position
layout(set=0, binding = 1) uniform sampler2D texSampler1; // normal
layout(set=0, binding = 2) uniform sampler2D texSampler2; // albedo-spec
layout(set=0, binding = 3) uniform sampler2D texSampler3; // depth?

//========================================================================
// Push constants. This should be in all stages.
//========================================================================
layout(push_constant) uniform PushConsts {
        int layer;
} pushConsts;


struct light_data_t
{
    vec4 position;
    vec4 color;
    vec4 attenuation; //[constant, linear, quad, cutoff]
};


layout(set=1, binding = 0, std140) uniform UniformBufferObject {
    vec2 num_lights;
    vec2 num_lights2;
    light_data_t light[10];
} u_lights;

vec3 diffuse_light(vec3 surface_normal,  // outward surface normal
                   vec3 light_direction, // vector from frag position to light
                   vec3 surface_color,
                   vec3 light_color
                   )
{
    // diffuse
    float diff = max(dot(surface_normal, light_direction), 0.0);
    vec3 diffuse = light_color * diff * surface_color;
    return diffuse;
}

vec3 specular( vec3 view_direction, // vector f rom frag  position to camera
               vec3 light_direction,
               vec3 surface_normal,
               vec3 light_spec_color,
               vec3 surface_spec_color,
               float shininess
               )
{
 //   vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-light_direction, surface_normal);
    float spec      = pow(max(dot(view_direction, reflectDir), 0.0), shininess);
    vec3 specular   = light_spec_color * spec * surface_spec_color;
    return specular;
}

float attenuation(float distance, float a, float b, float c)
{
    //return distance < 3 ? 1.0 : 0.0;
    return 1.0 / (1.0 + distance*b + c*distance*distance) ;
}


void main() 
{    
    vec3 frag_position = texture(texSampler0, f_UV.xy).xyz;
    vec3 normal        = texture(texSampler1, f_UV.xy).xyz;
    vec3 surface_color = texture(texSampler2, f_UV.xy).xyz;
    vec3 surface_depth = texture(texSampler3, f_UV.xy).xyz;

    switch( pushConsts.layer )
    {
        case 0:  out_Color = vec4( frag_position , 1.0); return;
        case 1:  out_Color = vec4( normal , 1.0); return;
        case 2:  out_Color = vec4( surface_color , 1.0); return;
        case 3:  out_Color = vec4(surface_depth.rrr , 1.0); return;
        default: break;
    }

    //out_Color =  vec4(surface_color*0.1 , 1);

    vec3 color = surface_color * 0.01;
    int num =  int(u_lights.num_lights[0]);
    for(int i=0; i < num ; i++)
    {
        vec3 light_dir = (  u_lights.light[i].position.xyz - frag_position).xyz;

         float  a       = attenuation( length(light_dir), u_lights.light[i].attenuation[0] ,
                                                          u_lights.light[i].attenuation[1],
                                                          u_lights.light[i].attenuation[2]);
        //float  a       = attenuation( length(light_dir), 1 , 0.1, 0.01);

        color          += diffuse_light( normal, normalize(light_dir), surface_color, u_lights.light[i].color.xyz ) * a;
    }
    out_Color =   vec4(color,1);

}
