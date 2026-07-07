// this is for the ps2 version of the file

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <libgen.h>
#include <string.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "flashpck.h"

// cross platform mkdir
#ifdef _WIN32
    #include <direct.h>
    #define make_directory(path) _mkdir(path)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define make_directory(path) mkdir(path, 0777)
#endif

#pragma pack(push, 1) // All packed struct, don't let compiler align

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


//void set_og_base_address(PckData* data, uint32_t base_addr) {
//    data->og_base_address = base_addr;
//}

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
    printf("File %s is %d bytes with 0x%.8x base\n", filename, fileSize, fileHeader.baseAddress);
    data->data_size = fileSize - 128;
    if (data->data_size != fileHeader.dataSize) {
        printf("Invalid data size %d != %d\n", data->data_size, fileHeader.dataSize);
        return 1;
    }
    printf("Base address: %.8x\n", fileHeader.baseAddress);
    data->og_base_address = fileHeader.baseAddress;

    // Load data and close file
    fseek(fin, 128, SEEK_SET);
    data->actual_data = malloc(data->data_size);
    fread(data->actual_data, data->data_size, 1, fin);
    fclose(fin);
    printf("Data loaded at 0x%.16x\n", data->actual_data);
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

void print_color(RGBAColor c) {
    printf("\x1b[38;2;%d;%d;%dm", c.r, c.g, c.b);
    printf("#%.2X%.2X%.2X%.2X", c.r, c.g, c.b, c.a);
    printf("\x1b[0m");
}

// okay, if you are running this program on a file and
// see the message "unknown opcode", that means this
// avm1 instruction wasn't implemented, that's basically
// my workflow, since i dont know if MC3 implements the 
// whole set of AVM1 instruction, i had to implement them one by one
// if that happens to you, go to the flash7 spec 
// https://www.ics.agh.edu.pl/dydaktyka/mm/lato0405_inf_d/wyklady/w4/SWF7_specification.pdf
// search for this opcode, and implement it.
// you should add an entry in the AVM1opcodes enum 
// and add a new case in swfAction_parse_avm1 function
static void swfAction_parse_avm1(PckData* pck, AVM1Bytecode* code) {
    // skipping the first uint16    
    uint8_t* opcode = (uint8_t*)(code+1);
    uint8_t ended = 0;
    char** constants = NULL; 
    while(!ended) {
        printf("0x%.8x %.2x ", pckData_get_og_from_ptr(pck, opcode), *opcode);
        switch(*opcode) {
            case AC_END: // opcode 0x0
                ended = 1;
                printf("end\n");
                break;
            case AC_PLAY: // opcode 0x6
                opcode++;
                printf("play\n");
                break;
            case AC_STOP: // opcode 0x7
                printf("stop\n");
                opcode++;
                break;
            case AC_TOGGLE_QUALITY: // opcode 0x08
                opcode++;
                printf("toggle quality\n");
                break;
            case AC_STOP_SOUNDS: // opcode 0x09
                opcode++;
                printf("stop sounds\n");
                break;
            case AC_ADD: // opcode 0x0a
                opcode++;
                printf("math add\n");
                break;
            case AC_PUSH_DUPLICATE:
                opcode++;
                printf("push duplicate\n");
                break;
            case AC_SUBTRACT:
                opcode++;
                printf("math subtract\n");
                break;
            case AC_MULTIPLY:
                opcode++;
                printf("math multiply\n");
                break;
            case AC_DIVIDE:
                opcode++;
                printf("math divide\n");
                break;
            case AC_STRING_EQUALS: 
                opcode++;
                printf("string equals\n");
                break;
            case AC_OR:
                opcode++;
                printf("math or\n");
                break;
            case AC_NOT:
                opcode++;
                printf("math not\n");
                break;
            case AC_STRING_EXTRACT:
                opcode++;
                printf("string extract\n");
                break;
            case AC_TO_INT:
                opcode++;
                printf("to integer\n");
                break;
            case AC_GETVAR: // opcode 0x1c
                opcode++;
                printf("get variable\n");
                break;
            case AC_SETVAR: // opcode 0x1d
                opcode++;
                printf("set variable\n");
                break;
            case AC_STRINGADD: // opcode 0x21
                opcode++;
                printf("string concatenation\n");
                break;
            case AC_NEW_OBJ: // opcode 0x40
                opcode++;
                printf("new object\n");
                break;
            case AC_GET_MEMBER:
                opcode++;
                printf("get member\n");
                break;
            case AC_SET_MEMBER: // opcode 0x4f
                opcode++;
                printf("set member\n");
                break;
            case AC_ADD2:
                opcode++;
                printf("math add2\n");
                break;
            case AC_LESS2:
                opcode++;
                printf("math less2\n");
                break;
            case AC_EQUALS2: // opcode 0x49
                opcode++;
                printf("math equals2\n");
                break;
            case AC_INCREMENT: // opcode 0x50
                opcode++;
                printf("increment\n");
                break;
            case AC_CALLMETHOD:
                opcode++;
                printf("call method\n");
                break;
            case AC_GREATER: // opcode 0x67
                opcode++;
                printf("math greater\n");
                break;
            case AC_CONSTANTPOOL: { // opcode 0x88
                swfAction_ConstantPool *cp = (swfAction_ConstantPool *)opcode;
                printf("constant pool\n");

                char *constant_name = cp->strings;
                for (uint16_t i = 0; i < cp->constant_count; ++i) {
                    stbds_arrput(constants, constant_name);
                    //printf("%s\n", constant_name);
                    constant_name += strlen(constant_name) + 1;
                }
            
                opcode = (uint8_t *)(constant_name);
                break;
            }
            case AC_POP:
                printf("pop\n");
                opcode++;
                break;
            case AC_DECREMENT:
                printf("decrement\n");
                opcode++;
                break;
            case AC_TRACE:
                printf("debug trace\n");
                opcode++;
                break;
            case AC_PUSH: { // opcode 0x96
                swfAction_StackPush *push = (swfAction_StackPush *)opcode;
            
                uint8_t *p   = push->data;
                uint8_t *end = p + push->length;
            
                while (p < end) {
                    uint8_t type = *p++;
                
                    switch (type) {
                    case SP_CONSTANT8:
                        printf("push constant8 \"%s\"\n",
                               constants[*p]);
                        p += 1;
                        break;
                        
                    case SP_CONSTANT16: {
                        uint16_t idx = *(uint16_t *)p;
                        printf("push constant16 \"%s\"\n",
                               constants[idx]);
                        p += 2;
                        break;
                    }

                    case SP_STRING:
                        printf("push string \"%s\"\n", (char *)p);
                        p += strlen((char *)p) + 1;
                        break;

                    case SP_FLOAT: {
                        float value = *(float *)p;
                        printf("push float %f\n", value);
                        p += 4;
                        break;
                    }

                    case SP_DOUBLE: {
                        double value = *(double *)p;
                        printf("push double %f\n", value);
                        p += 8;
                        break;
                    }

                    case SP_INTEGER: {
                        int32_t value = *(int32_t *)p;
                        printf("push int %d\n", value);
                        p += 4;
                        break;
                    }

                    case SP_BOOLEAN:
                        printf("push bool %s\n", *p ? "true" : "false");
                        p += 1;
                        break;

                    case SP_REGISTER:
                        printf("push register %u\n", *p);
                        p += 1;
                        break;

                    case SP_NULL:
                        printf("push null\n");
                        break;

                    case SP_UNDEFINED:
                        printf("push undefined\n");
                        break;
                
                    default:
                        printf("unknown push type 0x%u\n", type);
                        exit(1);
                    }
                }
                opcode = end;
                break;
            }
            case AC_GOTO_FRAME:
                swfAction_gotoFrame* a_frame = (swfAction_gotoFrame*)opcode;
                printf("goto frame %d\n", a_frame->frame_idx);
                opcode = (uint8_t*)(a_frame+1);
                break;
            case AC_IF:
                swfAction_if* a_if = (swfAction_if*)opcode;
                printf("goto %d if 0\n", a_if->offset);
                opcode = (uint8_t*)(a_if+1);
                break;
            case AC_JUMP:
                swfAction_jump* a_jump = (swfAction_jump*)opcode;
                printf("goto %d\n", a_jump->offset);
                opcode = (uint8_t*)(a_jump+1);
                break;
            case AC_WAIT_FOR_FRAME: // opcode 0x8a
                swfAction_WaitForFrame* wait_ff = (swfAction_WaitForFrame*)opcode;

                printf("wait for frame %d, skip %d codes\n", wait_ff->frame, wait_ff->skip_count);
                opcode = (uint8_t*)(wait_ff+1);
                break;
            default:
                printf("unknown opcode\n", *opcode);
                exit(1);
                break;
        }
    }
    stbds_arrfree(constants);
}

static uint8_t number_in(uint16_t number, uint16_t* arr, uint16_t count) {
    for(int i = 0; i < count; i++)
        if(number == arr[i]) return 1;
    return 0;
}

static void swfBITMAP_extract_4bpp(PckData* pck, bmpInfo1* info, RGBAColor** colors) {
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

static void swfBITMAP_extract8bpp(PckData* pck, bmpInfo1 *info, RGBAColor **colors, uint8_t swizzle)
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

void swfBITMAP_extract(PckData* pck, swfBITMAP* bmp, char* filepath) {
    int w = bmp->width, h = bmp->height;
    bmpInfo1* info1 = pckData_get_ptr_from_og(pck, bmp->ptr_to_info1);
    ps2Texture* tex_data = pckData_get_ptr_from_og(pck, info1->ptr_to_tex_data);
    RGBAColor* colors = malloc(sizeof(RGBAColor)*w*h);
    switch(tex_data->texture_type) {
        case BPP4_UNSWIZZLED:
            swfBITMAP_extract_4bpp(pck, info1, &colors);
            break;
        case BPP8_INDEX_SWIZZLED_BUT_TEXTURE_NOT:
            swfBITMAP_extract8bpp(pck, info1, &colors, 0);
            break;
        case BPP8_BOTH_SWIZZLED:
            swfBITMAP_extract8bpp(pck, info1, &colors, 1);
            break;
        default:
            fprintf(stderr, "%.8x, unknown texture type\n",  pckData_get_og_from_ptr(pck, bmp));
            return;
    }
    stbi_write_png(filepath, w, h, 4, colors, 0);
}
