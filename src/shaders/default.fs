#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

layout(location = 0) out vec4 FragColor;

uniform float time;
uniform vec3 cameraPos;
uniform vec3 cameraDir;
uniform bool torchActivated;
uniform vec3 torchColor;
uniform float torchInnerCutoff;
uniform float torchOuterCutoff;
uniform float torchAttenuationConstant;
uniform float torchAttenuationLinear;
uniform float torchAttenuationQuadratic;
uniform vec3 ambientColor;
uniform float ambientIntensity;
uniform float specularShininess;
uniform float specularIntensity;
uniform float attenuationConstant;
uniform float attenuationLinear;
uniform float attenuationQuadratic;

uniform sampler2D texture_diffuse0;
uniform sampler2D texture_specular0;

// Define a Light struct to mirror the C++ structure
struct Light {
    vec3 position;
    vec3 color;
};

#define MAX_LIGHTS 32
uniform Light lights[MAX_LIGHTS];
uniform int numLights;

void main()
{
    float viewDistance = length(cameraPos - FragPos);
    if (viewDistance > 80.0)
        discard;

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(cameraPos - FragPos);
    vec3 Albedo = texture(texture_diffuse0, TexCoords).rgb;

    // Torchlight contribution
    vec3 torchLight;
    if (torchActivated)
    {
        // check if lighting is inside the spotlight cone
        float theta = dot(viewDir, normalize(-cameraDir));
        float epsilon = torchInnerCutoff - torchOuterCutoff;
        float intensity = clamp((theta - torchOuterCutoff) / epsilon, 0.0, 1.0);

        // Quadratic attenuation
        float torchAttenuation = 1.0 / (torchAttenuationConstant +
                                        torchAttenuationLinear * viewDistance +
                                        torchAttenuationQuadratic * viewDistance * viewDistance);
        // Diffuse
        float torchDiffuse = max(dot(norm, viewDir), 0.0);

        // Specular
        vec3 torchReflectDir = reflect(-viewDir, norm);
        float torchSpec = pow(max(dot(viewDir, torchReflectDir), 0.0), specularShininess); // Shininess factor
        torchLight = (ambientIntensity * ambientColor * Albedo +
                      torchDiffuse * torchColor * Albedo * intensity +
                      torchSpec * torchColor * specularIntensity * intensity) * torchAttenuation;
    }
    else
    {
        torchLight = (ambientIntensity * ambientColor * Albedo);
    }

    // Static lights contribution
    float flicker = 0.8 + 0.2 * sin(time * 10.0) * sin(time * 3.0);
    vec3 staticLights = vec3(0.0);
    for (int i = 0; i < numLights; i++)
    {
        float distance = length(lights[i].position - FragPos);
        if (distance > 50.0)
            continue;
        vec3 lightDir = normalize(lights[i].position - FragPos);
        // Quadratic attenuation
        float lightAttenuation = 1.0 / (attenuationConstant +
                                        attenuationLinear * distance +
                                        attenuationQuadratic * distance * distance);

        // Diffuse
        float lightDiffuse = max(dot(norm, lightDir), 0.0);

        // Specular
        vec3 reflectDir = reflect(-lightDir, norm);
        float lightSpec = pow(max(dot(viewDir, reflectDir), 0.0), specularShininess); // Shininess factor

        staticLights += (ambientIntensity * ambientColor * Albedo +
                         lightDiffuse * lights[i].color * Albedo +
                         lightSpec * lights[i].color * specularIntensity) * lightAttenuation * flicker;
    }

    // Combine torchlight and static lights
    vec3 result = torchLight + staticLights;

    FragColor = vec4(result, 1.0);
}