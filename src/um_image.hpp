#ifndef UM_IMAGE_HPP
#define UM_IMAGE_HPP

#include <stdio.h>
#include "um.hpp"

namespace um
{

struct TGAImageHeader
{
    u8 id_length;
    u8 colormap_type;
    u8 image_type;
    // Color Map Specification
    u16 first_entry_index;
    u16 colormap_length;
    u8 colormap_entry_size;
    // Image Specification
    u16 xorigin;
    u16 yorigin;
    u16 image_width;
    u16 image_height;
    u8 pixel_depth;
    u8 image_descriptor;
};

struct TGAImageFooter
{
    u32 extension_area_offset = 0;
    u32 developer_dir_offset = 0;
    u8  signature[16] = {
        'T', 'R', 'U', 'E', 'V', 'I', 'S', 'I', 'O', 'N', '-', 'X', 'F', 'I', 'L', 'E'
    };
    u8  reserved = '.';
    u8  zero_string_terminator = 0;
};

struct TGAImageGray
{
    TGAImageGray(u16 width, u16 height);
    ~TGAImageGray();

    void fill(u8 gray);
    void set(u16 x, u16 y, u8 shade);
    void write_to_file(const char* filename);

private:
    TGAImageHeader _header;
    TGAImageFooter _footer;
    // raw data stored for the image
    u8*            _data;

    void write_image_data(FILE* fp);
};

}

#endif // UM_IMAGE_HPP
