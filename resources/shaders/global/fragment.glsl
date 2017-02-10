#version 330 core

struct vx_light
{
    vec3 position;
    vec3 color;
};

struct vx_material
{
    vec3 ambientColor;
    vec3 diffuseColor;
    vec3 specularColor;
    float shininess;
};

out vec4 color;

in vec3 fragNormal;
in vec3 fragPosition;

uniform vec3 cameraPosition;
uniform vx_material material;
uniform vx_light light;

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 fragToLight = normalize(light.position - fragPosition);
    // ===========================
    // ambient component
    // ===========================
    vec3 ambientComponent = material.ambientColor * 0.4f;
    // ===========================
    // diffuse component
    // ===========================
    vec3 diffuseComponent = material.diffuseColor * max(dot(normal, fragToLight), 0.0f);
    // ===========================
    // specular component
    // ===========================
    vec3 fragToCamera = normalize(cameraPosition - fragPosition);
    vec3 specularComponent = material.specularColor * pow(
        max(dot(reflect(-fragToLight, normal), fragToCamera), 0.0f),
        material.shininess
    );

    color = vec4(light.color, 1.0f) *
        vec4(ambientComponent + diffuseComponent + specularComponent, 1.0f);
}
