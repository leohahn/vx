#include <GL/glew.h>
#include <string.h>
#include <linux/limits.h>
#include <libgen.h>
#include "um.hpp"
#include "vx_string_hashmap.hpp"
#include "vx_shader_manager.hpp"

#define NUM_BUCKETS 26

char*  shader_from_src(const char *filepath);
GLuint make_program   (const char* vertex_path, const char* fragment_path);

void
shader_free(void *data)
{
    vx::Shader* shader = (vx::Shader*)data;
    free(shader->name);
    vx::string_hashmap_free_with_fn(shader->locations, free);
    free(shader);
}

vx::ShaderManager::ShaderManager()
{
    this->shaders = vx::string_hashmap_new(NUM_BUCKETS);
}

vx::ShaderManager::~ShaderManager()
{
    vx::string_hashmap_free_with_fn(this->shaders, shader_free);
}

vx::Shader*
vx::ShaderManager::load_program(const char* shader_name)
{
    if (vx::string_hashmap_is_present(this->shaders, shader_name))
    {
        vx::Shader *shader = (vx::Shader*)vx::string_hashmap_get(this->shaders, shader_name);
        return shader;
    }

    const char* vertex = "/vertex.glsl";
    const char* fragment = "/fragment.glsl";

    u32 shaderFolderLen = strlen(SHADERS_PATH) + strlen(shader_name);
    u32 vertexLen = shaderFolderLen + strlen(vertex) + 1;
    u32 fragmentLen = shaderFolderLen + strlen(fragment) + 1;

    char vertexPath[vertexLen];
    char fragmentPath[fragmentLen];
    // Set both strings to 0
    memset(vertexPath, 0, sizeof(char) * vertexLen);
    memset(fragmentPath, 0, sizeof(char) * fragmentLen);
    // Build vertex shader path
    strcat(vertexPath, SHADERS_PATH);
    strcat(vertexPath, shader_name);
    strcat(vertexPath, vertex);
    // Build fragment shader path
    strcat(fragmentPath, SHADERS_PATH);
    strcat(fragmentPath, shader_name);
    strcat(fragmentPath, fragment);

    DEBUG("NEW PROGRAM:\n");
    DEBUG("name: %s\n", shader_name);
    DEBUG("vertex: %s\n", vertexPath);
    DEBUG("fragment: %s\n", fragmentPath);

    GLuint program = make_program(vertexPath, fragmentPath);
    //
    // Alloc new shader
    //
    vx::Shader* shader = (vx::Shader*)malloc(sizeof(*shader));
    shader->name = (char*)calloc(strlen(shader_name) + 1, sizeof(char));
    shader->program = program;
    strcpy(shader->name, shader_name);
    shader->locations = vx::string_hashmap_new(16);

    DEBUG("id: %u\n\n", program);
    vx::string_hashmap_insert(this->shaders, shader_name, (void*)shader);
    return shader;
}

char *
shader_from_src(const char *filepath)
{
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Cannot open file %s\n", filepath);
        ASSERT(false == true);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    long int size = ftell(fp);
    rewind(fp);
    char *file_contents = (char*)malloc(sizeof(char) * size + 1);
    // char *file_contents = malloc(size * sizeof(char) + 1);
    file_contents[size] = 0;
    fread(file_contents, sizeof(char), size, fp);
    fclose(fp);
    return file_contents;
}

GLuint
make_program(const char* vertex_path, const char* fragment_path)
{
    // Fetch source codes from each shader
    const char *vertex_src = shader_from_src(vertex_path);
    const char *fragment_src = shader_from_src(fragment_path);

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint program;
    // Stores information about the compilation, so we can print it,
    GLchar info[512];
    // Flag to see if compilation or linking was successful.
    GLint success;

    if (vertex_src && fragment_src)
    {
        glShaderSource(vertex_shader, 1, &vertex_src, NULL);
        glShaderSource(fragment_shader, 1, &fragment_src, NULL);
    }
    else
    {
        goto cleanup;
    }

    /* Debug("Compiling vertex shader ... "); */
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(vertex_shader, 512, NULL, info);
        DEBUG("ERROR: Vertex shader compilation failed:\n");
        DEBUG("%s\n", info);
        goto cleanup;
    }
    /* printf("done\n"); */

    /* Debug("Compiling fragment shader ... "); */
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(fragment_shader, 512, NULL, info);
        DEBUG("ERROR: Fragment shader compilation failed:\n");
        DEBUG("%s\n", info);
        goto cleanup;
    }
    /* printf("done\n"); */

    program = glCreateProgram();
    /* Debug("Linking shader program ... "); */
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(fragment_shader, 512, NULL, info);
        DEBUG("ERROR: Fragment shader compilation failed:\n");
        DEBUG("%s\n", info);
        goto cleanup;
    }
    /* printf("done\n\n"); */

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    free((void*)vertex_src);
    free((void*)fragment_src);
    return program;

cleanup:
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    free((void*)vertex_src);
    free((void*)fragment_src);
    return -1;
}

void
vx::Shader::load_uniform_location(const char* uniform_name)
{
    GLuint* loc = (GLuint*)malloc(sizeof(GLuint));
    GLuint localLoc = glGetUniformLocation(this->program, uniform_name);
    *loc = localLoc;
    vx::string_hashmap_insert(this->locations, uniform_name, (void*)loc);
}

GLuint
vx::Shader::uniform_location(const char* uniform_name) const
{
    GLuint *loc = (GLuint*)vx::string_hashmap_get(this->locations, uniform_name);
    return (*loc);
}
