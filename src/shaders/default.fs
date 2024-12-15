#version 330 core

in vec3 FragColor;  // Color from the vertex shader (lighting)
in vec2 TexCoords;  // Texture coordinates from the vertex shader

out vec4 color;  // Final color to output

uniform sampler2D image;  // Texture for the object

// Constants for smoothstep-based distance falloff
uniform float smoothstepEdge0; // Inner boundary of the smoothstep function
uniform float smoothstepEdge1; // Outer boundary of the smoothstep function

void main()
{
    // Sample the texture at the given coordinates
    vec3 texColor = texture(image, TexCoords).rgb;

    // Calculate the linear depth from the depth buffer
    float distance = gl_FragCoord.z / gl_FragCoord.w;

    // Smoothstep to fade color based on distance
    // As distance increases, the color fades to black
    float fadeFactor = smoothstep(20.0, 0.1, distance);

    // Apply the light intensity to the texture color
    vec3 finalColor = texColor * FragColor * fadeFactor;

    // Output the final color
    color = vec4(finalColor, 1.0);
}