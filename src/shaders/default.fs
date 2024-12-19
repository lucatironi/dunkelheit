#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 cameraPos;       // Camera/torch position
uniform vec3 lightColor;      // Base color of the torchlight
uniform float lightRadius;    // Radius of the torchlight's effective area
uniform float time;           // Time for flickering effect
uniform sampler2D texture1;   // Object texture

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
    vec3 texColor = texture(texture1, TexCoord).rgb;

    // Flicker calculation
    float flicker = 0.8 + 0.2 * sin(time * 10.0) * sin(time * 3.0);

    // Torchlight contribution
    vec3 norm = normalize(Normal);
    vec3 torchDir = normalize(cameraPos - FragPos);
    float distanceToTorch = length(FragPos - cameraPos);
    float torchAttenuation = 1.0 - clamp(distanceToTorch / lightRadius, 0.0, 1.0);
    float torchDiffuse = max(dot(norm, torchDir), 0.0);
    vec3 torchLight = (0.2 * texColor + torchDiffuse * lightColor * texColor) * torchAttenuation;

    // Static lights contribution
    vec3 staticLight = vec3(0.0);
    for (int i = 0; i < numLights; i++) {
        vec3 lightDir = normalize(lights[i].position - FragPos);
        float distance = length(lights[i].position - FragPos);
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance); // Quadratic attenuation

        float diffuse = max(dot(norm, lightDir), 0.0);
        staticLight += (0.2 * texColor + diffuse * lights[i].color * texColor) * attenuation * flicker;
    }

    // Combine torchlight and static lights
    vec3 result = torchLight + staticLight;

    FragColor = vec4(result, 1.0);
}