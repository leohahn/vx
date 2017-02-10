#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out vec3 fragNormal;
out vec3 fragPosition;

uniform mat4 projection = mat4(1.0f);
uniform mat4 view = mat4(1.0f);
uniform mat4 model = mat4(1.0f);

void main()
{
    fragPosition = vec3(model * vec4(position, 1.0f));
    fragNormal = mat3(transpose(inverse(model))) * normal;

    // Lighting calculations have to be done with world coordinates.
    gl_Position = projection * view * model * vec4(position, 1.0f);
}
