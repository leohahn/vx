#version 330 core

in vec2 texCoords;
out vec4 color;

uniform sampler2D fontAtlas;
uniform vec3 textColor = vec3(0.0f, 0.0f, 0.0f);

void main()
{
    vec4 sampled = vec4(textColor, texture(fontAtlas, texCoords).a);
    // color = vec4(textColor, 1.0f) * sampled;
    // color = vec4(1.0f, 1.0f, 0.0f, 1.0f);
    color = sampled;
}
