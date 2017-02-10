#include "vx_files.hpp"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <FreeImage.h>
#include "um.hpp"

#define FONTS_PATH "resources/fonts/"

void
vx::FntFile::load_texture_from_png(const char* fontname)
{
    const char* ext = ".png";
    // =====================================
    // Creates the full path of the file to load
    // =====================================
    u32 size = strlen(FONTS_PATH) + strlen(fontname) + strlen(ext);
    char fullpath[size];
    memset(fullpath, 0, sizeof(char) * size);
    strcat(fullpath, FONTS_PATH);
    strcat(fullpath, fontname);
    strcat(fullpath, ext);
    // Open the file and check for errors
    FILE *fp = fopen(fullpath, "r");
    ASSERT(fp != NULL);
    // =====================================
    // Starts reading the PNG file
    // =====================================
    FreeImage_Initialise(TRUE);

    FIBITMAP *bitmap = FreeImage_Load(FIF_PNG, fullpath, PNG_DEFAULT);
    ASSERT(bitmap != NULL);

    this->image_width = FreeImage_GetWidth(bitmap);
    this->image_height = FreeImage_GetHeight(bitmap);
    u8 *imagebits = FreeImage_GetBits(bitmap);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment restriction
    glGenTextures(1, &this->texture_id);
    glBindTexture(GL_TEXTURE_2D, this->texture_id);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,                   // the mipmap level (dont care)
        GL_RGBA,             // We store the image in RGBA values
        this->image_width,
        this->image_height,
        0,                   // Always 0, legacy stuff!
        GL_RGBA,             // The format of the loaded image
        GL_UNSIGNED_BYTE,    // data type of the loaded image
        imagebits            // pointer to the image data
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    FreeImage_Unload(bitmap);
    FreeImage_DeInitialise();
}

vx::FntFile::FntFile(const char *fontname)
{
    const char* ext = ".fnt";
    // Creates the full path of the file to load
    u32 size = strlen(FONTS_PATH) + strlen(fontname) + strlen(ext);
    char fullpath[size];
    memset(fullpath, 0, sizeof(char) * size);
    strcat(fullpath, FONTS_PATH);
    strcat(fullpath, fontname);
    strcat(fullpath, ext);
    DEBUG("Loading font %s from %s\n", fontname, fullpath);
    // opens the file and asserts that there is no error.
    FILE *fp = fopen(fullpath, "r");
    ASSERT(fp != NULL);
    // buffer we use to read a line from the file.
    char buf[1000] = {0};

    u32 id, x, y, width, height, xadvance;
    i32 xoffset, yoffset;

    while (fgets(buf, 1000, fp) != NULL)
    {
        sscanf(buf, "chars count=%lu", &this->num_characters);
        sscanf(buf, "common lineHeight=%lu", &this->line_height);

        u8 matched = sscanf(
            buf,
            "char id=%u x=%u y=%u width=%u height=%u xoffset=%d yoffset=%d xadvance=%u",
            &id, &x, &y, &width, &height, &xoffset, &yoffset, &xadvance
        );
        // If it didnt match all of the variables (8), ignore this line.
        if (matched != 8) continue;

        // If it gets here, it means we can create the vx_fnt_char structure
        ASSERT(id <= 127);
        this->characters[id].id = id;
        this->characters[id].x = x;
        this->characters[id].y = y;
        this->characters[id].width = width;
        this->characters[id].height = height;
        this->characters[id].xoffset = xoffset;
        this->characters[id].yoffset = yoffset;
        this->characters[id].xadvance = xadvance;
    }
    fclose(fp);
    // Now the fnt file was sucessfully parsed, we will load the .png file as a texture.
    load_texture_from_png(fontname);
}
