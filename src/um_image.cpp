#include "um_image.hpp"
#include <cstring>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>

void
write_header(const um::TGAImageHeader& header, FILE* fp)
{
    if (!um::is_little_endian())
    {
        printf("Implement this function for big endian machines, im lazy :(\n");
        abort();
    }
    const u8 NUM_BYTES_HEADER = 18;
    u8 buf[NUM_BYTES_HEADER];

    u32 offset; // the offset inside of the buffer

    offset = 0;
    memcpy(buf + offset, &header.id_length, sizeof(u8));
    offset = 1;
    memcpy(buf + offset, &header.colormap_type, sizeof(u8));
    offset = 2;
    memcpy(buf + offset, &header.image_type, sizeof(u8));
    offset = 3;
    memcpy(buf + offset, &header.first_entry_index, sizeof(u16));
    offset = 5;
    memcpy(buf + offset, &header.colormap_length, sizeof(u16));
    offset = 7;
    memcpy(buf + offset, &header.colormap_entry_size, sizeof(u8));
    offset = 8;
    memcpy(buf + offset, &header.xorigin, sizeof(u16));
    offset = 10;
    memcpy(buf + offset, &header.yorigin, sizeof(u16));
    offset = 12;
    memcpy(buf + offset, &header.image_width, sizeof(u16));
    offset = 14;
    memcpy(buf + offset, &header.image_height, sizeof(u16));
    offset = 16;
    memcpy(buf + offset, &header.pixel_depth, sizeof(u8));
    offset = 17;
    memcpy(buf + offset, &header.image_descriptor, sizeof(u8));

    fwrite(buf, sizeof(u8), NUM_BYTES_HEADER, fp);
}

void
write_footer(const um::TGAImageFooter& footer, FILE* fp)
{
    fwrite(&footer.extension_area_offset, sizeof(u32), 1, fp);
    fwrite(&footer.developer_dir_offset, sizeof(u32), 1, fp);
    fwrite(footer.signature, sizeof(u8), 16, fp);
    fwrite(&footer.reserved, sizeof(u8), 1, fp);
    fwrite(&footer.zero_string_terminator, sizeof(u8), 1, fp);
}

/* ---------------------------------------------------------------
                      TGA Image Grayscale
 * --------------------------------------------------------------- */
um::TGAImageGray::TGAImageGray(u16 width, u16 height)
{
    _header.id_length = 0;
    _header.colormap_type = 0;
    _header.image_type = 2;
    _header.first_entry_index = 0;
    _header.colormap_length = 0;
    _header.colormap_entry_size = 0;
    _header.xorigin = 0;
    _header.yorigin = 0;
    _header.image_width = width;
    _header.image_height = height;
    _header.pixel_depth = 8;
    _header.image_descriptor = 0;
    _header.image_descriptor = 0;

    _data = new u8[width * height]();
}

um::TGAImageGray::~TGAImageGray()
{
    delete[] _data;
}

void
um::TGAImageGray::set(u16 x, u16 y, u8 shade)
{
    u32 index = (y * _header.image_width) + x;
    ASSERT(index < (u32)_header.image_height * (u32)_header.image_width);
    _data[index] = shade;
}

void
um::TGAImageGray::write_to_file(const char* filename)
{
    FILE* fp = fopen(filename, "w");
    if (fp == nullptr)
    {
        printf("Could not open %s", filename);
        abort();
    }

    write_header(_header, fp);
    write_image_data(fp);
    write_footer(_footer, fp);

    fclose(fp);
}

void
um::TGAImageGray::write_image_data(FILE* fp)
{
    // The bits are stored in the following order: ARGB
    i32 n = fwrite(_data, sizeof(u8), _header.image_width * _header.image_height, fp);
    if (n < 0)
    {
        printf("Error writing to file, aborting\n");
        abort();
    }
}

void
um::TGAImageGray::fill(u8 gray)
{
    for (i32 i = 0; i < _header.image_width * _header.image_height; i++)
    {
        _data[i] = gray;
    }
}
