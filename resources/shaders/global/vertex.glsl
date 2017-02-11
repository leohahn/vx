#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out vec3 frag_normal;
out vec3 frag_position;

// uniform mat4 projection = mat4(1.0f);
// uniform mat4 view = mat4(1.0f);
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model = mat4(1.0f);

void main()
{
    frag_position = vec3(model * vec4(position, 1.0f));
    frag_normal = mat3(transpose(inverse(model))) * normal;

    gl_Position = projection * view * model * vec4(position, 1.0f);
}
