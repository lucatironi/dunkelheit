#version 330 core

in vec2 TexCoords;

uniform sampler2D image;

out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}