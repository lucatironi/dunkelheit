#version 330 core

in vec3 VertexLight;
in vec2 TexCoords;

uniform sampler2D image;

out vec4 FragColor;

void main()
{
    vec4 tex = texture(image, TexCoords);
    FragColor = tex * vec4(VertexLight, 1.0);
    float distance = gl_FragCoord.z / gl_FragCoord.w;
    FragColor.rgb *= smoothstep(10.0, 0.1, distance);
}