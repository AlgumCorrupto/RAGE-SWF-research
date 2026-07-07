// this is for the ps2 version of the file

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <libgen.h>
#include <string.h>


#include "flashpck.h"


// just to make things more clear
// when i refer to "character id", "object id" or whatever id,
// i'm referring to its position in the swfOBJECTS array pointed by swfFILE.
// capiche?

// the interesting and painfull thing about the *ck files of this game is that
// its literally a live dump of C++ objects, its not binary encoded data
// that its parsed in some way. The game reads the contents as a whole
// and just fixes the pointers that need to be fixed based on its base address
// which is contained a tiny 0x10 bytes header at the start of the file.
// this type of serialization is explained in this article
// http://tomhulton.blogspot.com/2011/12/load-in-place-data-structures-and.html

char swfObjectTypesString[10][16] = {
    "header", "swfSHAPE", "swfSPRITE", "swfBUTTON", "swfBITMAP", "swfFONT", "swfTEXT", "swfEDITTEXT", "swfSOUND", "swfMORPHSHAPE"
};


char swfCmdTypesString[5][32] = {
    "swfPlaceObject2", "swfClipEvent", "swfRemoveObject2", "swfCMD_doAction", "swfDoInitAction" // not sure about the last one
};

void *pckData_get_ptr_from_og(PckData *data, uint32_t og_addr)
{
    return (uint8_t *)data->actual_data +
           (og_addr - data->og_base_address);
}

uint32_t pckData_get_og_from_ptr(PckData* data, void* ptr) {
    return data->og_base_address + ((uint8_t*) ptr - (uint8_t*)data->actual_data);
}

uint32_t pckData_get_rel_from_og(PckData* data, uint32_t og_addr) {
    return og_addr - data->og_base_address;
} 

swfOBJECT_header *pckData_get_obj(PckData *data, int pos)
{
    swfFILE *file = data->actual_data;

    if (pos < 1 || pos >= file->objectCount)
        return NULL;

    uint32_t *list =
        pckData_get_ptr_from_og(data, file->pointToObjectPtrList);

    uint32_t obj_addr = list[pos];

    return pckData_get_ptr_from_og(data, obj_addr);
}

int pckData_init(PckData* data, char* filename) {
    FILE *fin = fopen(filename, "rb");
    PckFileHeader fileHeader;
    fseek(fin, 0, SEEK_END);
    uint32_t fileSize = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    fread(&fileHeader, sizeof(PckFileHeader), 1, fin);
    data->data_size = fileSize - 128;
    if (data->data_size != fileHeader.dataSize) {
        printf("Invalid data size %d != %d\n", data->data_size, fileHeader.dataSize);
        return 1;
    }
    data->og_base_address = fileHeader.baseAddress;

    // Load data and close file
    fseek(fin, 128, SEEK_SET);
    data->actual_data = malloc(data->data_size);
    fread(data->actual_data, data->data_size, 1, fin);
    fclose(fin);
    return 0;
}

void pckData_free(PckData* data) {
    free(data->actual_data);
}

RGBAColor add_color(RGBAColor c1, RGBAColor c2) {
    return (RGBAColor){
        .a = c1.a + c2.a,
        .b = c1.b + c2.b,
        .g = c1.g + c2.g,
        .r = c1.r + c2.r,
    };
}

RGBAColor mult_color(RGBAColor c1, RGBAColor c2) {
    return (RGBAColor){
        .a = c1.a * c2.a,
        .b = c1.b * c2.b,
        .g = c1.g * c2.g,
        .r = c1.r * c2.r,
    };
}

RGBAColor xform_color(RGBAColor c, swfCXFORMWITHAPLHA x) {

    return add_color(x.add_term, mult_color(c, x.mult_term));
}


static uint8_t number_in(uint16_t number, uint16_t* arr, uint16_t count) {
    for(int i = 0; i < count; i++)
        if(number == arr[i]) return 1;
    return 0;
}

void swfBITMAP_extract_4bpp(PckData* pck, bmpInfo1* info, RGBAColor** colors) {
    ps2Texture* data = (ps2Texture*) pckData_get_ptr_from_og(pck, info->ptr_to_tex_data);
    uint32_t num_pixels = data->height * data->width;
    ps2IndexedColors* idx_colors = (ps2IndexedColors*) pckData_get_ptr_from_og(pck, info->indexed_colors_ptr);
    uint8_t* tex = pckData_get_ptr_from_og(pck, data->ptr_to_texture);
    *colors = malloc(num_pixels * sizeof(RGBAColor));
    // this method only extracts images encoded with
    // as 4 bits per pixel indexed image
    // and i managed to successfully extract a single image
    // from legal.pck this way
    for (size_t pos = 0; pos < (num_pixels + 1) / 2; pos++) {
    
        uint8_t byte = tex[pos];
    
        uint8_t idx0 = byte & 0x0F;
        uint8_t idx1 = (byte >> 4) & 0x0F;
    
        size_t p0 = pos * 2;
        size_t p1 = p0 + 1;
    
        (*colors)[p0] = idx_colors->indexed_colors[idx0];

        if (p1 < num_pixels) {
            (*colors)[p1] = idx_colors->indexed_colors[idx1];
        }
    }
}

static void ps2_convert_palette32(const RGBAColor *src,
                                  RGBAColor *dst,
                                  size_t color_count)
{
    const int stripes = 2;
    const int colors  = 8;
    const int blocks  = 2;

    size_t index = 0;
    size_t parts = color_count / 32;

    for (size_t part = 0; part < parts; part++) {
        for (int block = 0; block < blocks; block++) {
            for (int stripe = 0; stripe < stripes; stripe++) {
                for (int color = 0; color < colors; color++) {

                    size_t palette_index =
                        part * 32 +
                        block * 8 +
                        stripe * 16 +
                        color;

                    dst[index++] = src[palette_index];
                }
            }
        }
    }
}



static void swfBITMAP_unswizzle8(PckData* pck, uint8_t *dst,
                       const uint8_t *src,
                       int width,
                       int height)
{
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            int block_location =
                (y & (~0xF)) * width +
                (x & (~0xF)) * 2;

            int swap_selector =
                (((y + 2) >> 2) & 1) * 4;

            int pos_y =
                (((y & (~3)) >> 1) + (y & 1)) & 7;

            int column_location =
                pos_y * width * 2 +
                ((x + swap_selector) & 7) * 4;

            int byte_num =
                ((y >> 1) & 1) +
                ((x >> 2) & 2);

            dst[y * width + x] =
                src[block_location + column_location + byte_num];
        }
    }
}

void swfBITMAP_extract_8bpp(PckData* pck, bmpInfo1 *info, RGBAColor **colors, uint8_t swizzle)
{
    ps2Texture *data =
        pckData_get_ptr_from_og(pck, info->ptr_to_tex_data);

    ps2IndexedColors *idx_colors =
        pckData_get_ptr_from_og(pck, info->indexed_colors_ptr);

    size_t width  = data->width;
    size_t height = data->height;
    size_t num_pixels = width * height;

    uint8_t *swizzled =
        pckData_get_ptr_from_og(pck, data->ptr_to_texture);

    uint8_t *indices = malloc(num_pixels);
    *colors = malloc(num_pixels * sizeof(RGBAColor));
    
    if (!indices || !*colors) {
        free(indices);
        free(*colors);
        *colors = NULL;
        return;
    }
    
    if (swizzle) {
        swfBITMAP_unswizzle8(pck, indices, swizzled, width, height);
    } else {
        memcpy(indices, swizzled, num_pixels);
    }

    /* Unswizzle the 256-color CLUT */
    RGBAColor palette[256];
    ps2_convert_palette32(idx_colors->indexed_colors, palette, 256);

    /* Apply palette */
    for (size_t i = 0; i < num_pixels; i++) {
        (*colors)[i] = palette[indices[i]];
    }

    free(indices);
}


