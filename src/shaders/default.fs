#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

layout(location = 0) out vec4 FragColor;

uniform float time;
uniform vec3 cameraPos;
uniform vec3 torchPos;
uniform vec3 torchDir;
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

vec3 CalcBlinnPhong(vec3 viewDir, vec3 normal, vec3 lightDir, vec3 lightColor, float intensity)
{
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    // Specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), specularShininess);

    return (ambientColor * ambientIntensity + diff * lightColor * intensity + spec * lightColor * specularIntensity * intensity);
}

float CalcAtt(float distance, float constant, float linear, float quadratic)
{
    return 1.0 / (constant + linear * distance + quadratic * distance * distance);
}

void main()
{
    float torchDist = length(cameraPos - FragPos);
    if (torchDist > 80.0)
        discard;

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(cameraPos - FragPos);
    vec3 Albedo = texture(texture_diffuse0, TexCoords).rgb;

    // Torchlight contribution
    vec3 torchLight;
    if (torchActivated)
    {
        vec3 lightDir = normalize(torchPos - FragPos);
        // Spotlight Intensity
        float theta = dot(lightDir, normalize(-torchDir));
        float epsilon = torchInnerCutoff - torchOuterCutoff;
        float intensity = clamp((theta - torchOuterCutoff) / epsilon, 0.0, 1.0);

        torchLight = CalcBlinnPhong(lightDir, norm, lightDir, torchColor, intensity) *
                    CalcAtt(torchDist, torchAttenuationConstant, torchAttenuationLinear, torchAttenuationQuadratic) *
                    Albedo;
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
        float lightDist = length(lights[i].position - FragPos);
        if (lightDist > 50.0)
            continue;
        vec3 lightDir = normalize(lights[i].position - FragPos);

        staticLights += CalcBlinnPhong(viewDir, norm, lightDir, lights[i].color, 1.0) *
                        CalcAtt(lightDist, attenuationConstant, attenuationLinear, attenuationQuadratic) *
                        flicker *
                        Albedo;
    }

    // Combine torchlight and static lights
    vec3 result = torchLight + staticLights;

    FragColor = vec4(result, 1.0);
}