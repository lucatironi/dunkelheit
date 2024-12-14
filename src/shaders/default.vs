#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

struct Light {
    vec3 position;
    vec3 color;
    float attenuation;
};

const int MAX_LIGHTS = 32;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 cameraPos;
uniform vec3 lightColor;
uniform float linearAtt;
uniform float constantAtt;
uniform float quadraticAtt;
uniform Light lights[MAX_LIGHTS];

out vec3 VertexLight;
out vec2 TexCoords;

vec3 CalcPointLight(vec3 lightPos, vec3 vertexPos, vec3 lightColor);

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_Position = projection * view * worldPos;

    // ambient light
    vec3 ambient = vec3(0.1);

    VertexLight = ambient + CalcPointLight(cameraPos, worldPos.xyz, lightColor);

    for (int i = 0; i < lights.length(); i++)
    {
        VertexLight += CalcPointLight(lights[i].position, worldPos.xyz, lights[i].color);
    }

    TexCoords = aTexCoords;
}

vec3 CalcPointLight(vec3 lightPos, vec3 vertexPos, vec3 lightColor)
{
    float attenuation = 0.001;
    vec3 normal = normalize(aNormal);
    vec3 lightDir = normalize(lightPos - vertexPos);
    float diffuse = max(dot(normal, lightDir), 0.0);
    float distance = length(lightPos -  vertexPos);
    if (distance < 6)
        attenuation = 1.0 / (constantAtt + linearAtt * distance + quadraticAtt * (distance * distance));

    return (lightColor * attenuation) + (diffuse * attenuation);
}