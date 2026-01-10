#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

layout(location = 0) out vec4 FragColor;

uniform bool menuActive;
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

struct Light {
    vec3 position;
    vec3 color;
};

#define MAX_LIGHTS 32
uniform Light lights[MAX_LIGHTS];
uniform int numLights;

// Fog Uniforms
uniform vec3 fogColor = vec3(0.05, 0.05, 0.08);
uniform float fogDensity = 0.15;

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
    float pixelDist = length(cameraPos - FragPos);
    if (pixelDist > 80.0)
        discard;

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(cameraPos - FragPos);
    vec4 texColor = texture(texture_diffuse0, TexCoords);
    vec3 Albedo = texColor.rgb;
    float alpha = texColor.a;

    // --- 1. LIGHTING CALCULATION ---
    vec3 torchLight = vec3(0.0);
    float airGlow = 0.0; // For volumetric fog effect

    if (torchActivated)
    {
        vec3 lightDir = normalize(torchPos - FragPos);
        float theta = dot(lightDir, normalize(-torchDir));
        float epsilon = torchInnerCutoff - torchOuterCutoff;
        float intensity = clamp((theta - torchOuterCutoff) / epsilon, 0.0, 1.0);
        float atten = CalcAtt(pixelDist, torchAttenuationConstant, torchAttenuationLinear, torchAttenuationQuadratic);

        torchLight = CalcBlinnPhong(viewDir, norm, lightDir, torchColor, intensity) * atten * Albedo;
        airGlow = intensity * atten; // Strength of light hitting the dust in the air
    }
    else
    {
        torchLight = (ambientIntensity * ambientColor * Albedo);
    }

    float flicker = 0.8 + 0.2 * sin(time * 10.0) * sin(time * 3.0);
    vec3 staticLights = vec3(0.0);
    for (int i = 0; i < numLights; i++)
    {
        float lightDist = length(lights[i].position - FragPos);
        if (lightDist > 50.0) continue;
        vec3 lightDir = normalize(lights[i].position - FragPos);
        staticLights += CalcBlinnPhong(viewDir, norm, lightDir, lights[i].color, 1.0) *
                        CalcAtt(lightDist, attenuationConstant, attenuationLinear, attenuationQuadratic) *
                        flicker * Albedo;
    }

    vec3 combinedLight = torchLight + staticLights;
    // --- 1. IDENTIFY EYES AND BLOOM AREA ---
    vec3 glowKey = vec3(1.0, 0.0, 1.0); // Magenta mask
    float magentaDist = distance(Albedo, glowKey);

    // Core of the eye
    bool isEye = magentaDist < 0.4;
    // Halo/Bloom area around the eye
    float bloomArea = smoothstep(0.8, 0.3, magentaDist);

    // --- 2. CALCULATE FOG ---
    float fogFactor = 1.0 / exp(pixelDist * fogDensity);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    // --- 3. FINAL COLOR COMPOSITION ---
    vec3 finalWithFog;
    vec3 targetOrange = vec3(1.0, 0.5, 0.0);

    if (isEye)
    {
        // Ignores fog entirely and stays bright
        finalWithFog = targetOrange * 4.0 * flicker;
    }
    else
    {
        vec3 surfaceWithFog = mix(fogColor, combinedLight, fogFactor);

        // BLOOM EFFECT: Add an orange halo that "cuts" through the fog
        // We use 'bloomArea' to decide where the halo is,
        // and we mix it with the fogged surface.
        vec3 bloomGlow = targetOrange * bloomArea * 5.0 * flicker;

        // Make the bloom slightly affected by fog so it doesn't look like a 2D UI element
        // but keep it much stronger than the body (pow(fogFactor, 0.4))
        finalWithFog = surfaceWithFog + (bloomGlow * pow(fogFactor, 0.8));
    }

    // --- 4. VOLUMETRIC TORCH BEAM ---
    finalWithFog += torchColor * airGlow * (1.0 - fogFactor) * 0.2;

    if (menuActive)
    {
        float gray = dot(finalWithFog, vec3(0.3, 0.59, 0.11));
        finalWithFog = mix(finalWithFog, vec3(gray), 0.5);

        // 2. Tint it "Cold" (Blue/Cyan)
        finalWithFog *= vec3(0.7, 0.8, 1.0);

        // 3. Scanlines
        float scale = 4.0;
        float scanline = sin(gl_FragCoord.y / scale) * 0.15 + 0.85;

        finalWithFog *= scanline;
    }

    FragColor = vec4(finalWithFog, alpha);
}