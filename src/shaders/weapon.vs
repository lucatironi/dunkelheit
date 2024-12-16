#version 330 core

layout(location = 0) in vec3 inPosition;  // Vertex position

uniform mat4 model;     // Model transformation matrix
uniform mat4 view;      // View matrix
uniform mat4 projection; // Projection matrix

void main() {
    // Apply transformations
    gl_Position = projection * view * model * vec4(inPosition, 1.0);
}
