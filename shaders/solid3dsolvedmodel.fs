#version 450 core

out vec4 FragColor;

uniform vec3 viewPos;
uniform bool isSolidColor;
uniform bool isLightingOn;

in VertexData{
    vec3 color;
    vec3 normal;
    vec3 fragPos;
}vsout;

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};


uniform Material material;
uniform Light light;


void main()
{
    vec3 result; // final color

    if(isLightingOn)
    {
    // ambient
    vec3 ambient = light.ambient * material.ambient;

    // diffuse
    vec3 norm = normalize(vsout.normal);
    vec3 lightDir = normalize(light.position - vsout.fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * (diff * material.diffuse);

    // specular
    vec3 viewDir = normalize(viewPos - vsout.fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);



//    if (intensity > 0.95)
//            color = vec4(1.0,0.5,0.5,1.0);
//    else if (intensity > 0.5)
//            color = vec4(0.6,0.3,0.3,1.0);
//    else if (intensity > 0.25)
//            color = vec4(0.4,0.2,0.2,1.0);
//    else
//            color = vec4(0.2,0.1,0.1,1.0);


    // result
    result = (ambient + diffuse + specular)*vsout.color;
    }
    else
        result = vsout.color; // no lighting

    if(isSolidColor)
        result = vec3(1.0, 1.0, 1.0); // white

    FragColor = vec4(result, 1.0);
}
