#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set=0, binding = 0) uniform sampler2DArray texSampler;

layout(location = 1) in vec4 fragTexCoord;

layout(location = 0) out vec4 outColor;





vec3 diffuse_light(vec3 frag_position,
                   vec3 surface_normal, // outward surface normal
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
    float spec = pow(max(dot(view_direction, reflectDir), 0.0), shininess);
    vec3 specular = light_spec_color * spec * surface_spec_color;
    return specular;
}

void main()
{
    vec3 surface_color = texture(texSampler, fragTexCoord.rgb   ).rgb;



    outColor = vec4(surface_color,1);

}
/*
void main()
{
    // ambient
    vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;

    // attenuation
    float distance    = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    ambient  *= attenuation;
    diffuse   *= attenuation;
    specular *= attenuation;

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
*/
