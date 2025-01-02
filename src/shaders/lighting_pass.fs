#version 330 core

in vec2 TexCoords;

layout(location = 0) out vec4 FragColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;

uniform vec3 cameraPos;    // Camera/torch position
uniform vec3 lightColor;   // Base color of the torchlight
uniform float lightRadius; // Radius of the torchlight's effective area
uniform float time;        // Time for flickering effect
uniform float ambient = 0.5;
uniform float specularShininess = 4.0;
uniform float specularIntensity = 0.1;

// Define a Light struct to mirror the C++ structure
struct Light {
    vec3 position;
    vec3 color;
};

#define MAX_LIGHTS 10
uniform Light lights[MAX_LIGHTS];
uniform int numLights;

void main()
{
    // retrieve data from gbuffer
    vec3 FragWorldPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Albedo = texture(gAlbedo, TexCoords).rgb;

    // Flicker calculation
    float flicker = 0.8 + 0.2 * sin(time * 10.0) * sin(time * 3.0);

    // Torchlight contribution
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(cameraPos - FragWorldPos);
    vec3 torchDir = normalize(cameraPos - FragWorldPos);
    float distanceToTorch = length(cameraPos - FragWorldPos);
    float torchAttenuation = 1.0 - clamp(distanceToTorch / lightRadius, 0.0, 1.0);

    // Diffuse
    float torchDiffuse = max(dot(norm, torchDir), 0.0);

    // Specular
    vec3 torchReflectDir = reflect(-torchDir, norm);
    float torchSpec = pow(max(dot(viewDir, torchReflectDir), 0.0), specularShininess); // Shininess factor

    vec3 torchLight = (ambient * Albedo +
                       torchDiffuse * lightColor * Albedo +
                       torchSpec * lightColor * specularIntensity) * torchAttenuation;

    // Static lights contribution
    vec3 staticLights = vec3(0.0);
    for (int i = 0; i < numLights; i++)
    {
        vec3 lightDir = normalize(lights[i].position - FragWorldPos);
        float distance = length(lights[i].position - FragWorldPos);
        float lightAttenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance); // Quadratic attenuation

        // Diffuse
        float lightDiffuse = max(dot(norm, lightDir), 0.0);

        // Specular
        vec3 reflectDir = reflect(-lightDir, norm);
        float lightSpec = pow(max(dot(viewDir, reflectDir), 0.0), specularShininess); // Shininess factor

        staticLights += (ambient * Albedo +
                         lightDiffuse * lights[i].color * Albedo +
                         lightSpec * lights[i].color * specularIntensity) * lightAttenuation * flicker;
    }

    // Combine torchlight and static lights
    vec3 result = torchLight + staticLights;

    FragColor = vec4(result, 1.0);
}