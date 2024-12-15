#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

struct Light {
    vec3 position;
    vec3 color;
};

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform Light lights[32];
uniform int numLights;
uniform vec3 cameraPos;
uniform vec3 lightColor;
uniform float linearAtt;
uniform float constantAtt;
uniform float quadraticAtt;

out vec3 FragColor;
out vec2 TexCoords;

// Function to calculate attenuation for point lights
float calculateAttenuation(float distance) {
    return 1.0 / (constantAtt + linearAtt * distance + quadraticAtt * (distance * distance));
}

// Function to calculate diffuse and specular lighting from a point light
vec3 calculateLighting(vec3 lightPos, vec3 lightColor, vec3 normal, vec3 fragPos) {
    // Calculate light direction and distance
    vec3 lightDir = normalize(lightPos - fragPos);
    float distance = length(lightPos - fragPos);

    // Attenuation
    float attenuation = calculateAttenuation(distance);

    // Diffuse lighting (Lambertian reflection)
    float diff = max(dot(normal, lightDir), 0.0);

    // Specular lighting (Phong reflection model)
    vec3 viewDir = normalize(cameraPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);  // Reflection vector
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8);  // Specular exponent (shininess)

    // Combine diffuse and specular lighting
    vec3 diffuse = diff * lightColor * attenuation;
    vec3 specular = spec * lightColor * attenuation;

    return (diffuse + specular);
}

void main()
{
    // Transform vertex position to world space
    vec4 worldPos = model * vec4(aPos, 1.0);

    // Compute the final clip space position
    gl_Position = projection * view * worldPos;

    // Compute the normal in world space
    vec3 normal = normalize(mat3(transpose(inverse(model))) * aNormal);

    // Initialize the final color with ambient lighting
    vec3 color = vec3(0.2);  // Low-level ambient light

    // First, add the camera light (which is at cameraPos)
    color += calculateLighting(cameraPos, lightColor, normal, worldPos.xyz); // Using white light at cameraPos

    // Loop through all active lights and accumulate their contributions
    for (int i = 0; i < numLights; i++) {
        color += calculateLighting(lights[i].position, lights[i].color, normal, worldPos.xyz);
    }

    // Output the final computed color
    FragColor = color;

    // Pass the texture coordinates to the fragment shader
    TexCoords = aTexCoords;
}