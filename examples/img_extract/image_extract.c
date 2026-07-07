#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <libgen.h>
#include <string.h>


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "../../flashpck.h"

#ifdef _WIN32
    #include <direct.h>
    #define make_directory(path) _mkdir(path)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define make_directory(path) mkdir(path, 0777)
#endif

void to_png(PckData* pck, swfBITMAP* bmp, char* filepath) {
    int w = bmp->width, h = bmp->height;
    bmpInfo1* info1 = pckData_get_ptr_from_og(pck, bmp->ptr_to_info1);
    ps2Texture* tex_data = pckData_get_ptr_from_og(pck, info1->ptr_to_tex_data);
    RGBAColor* colors = malloc(sizeof(RGBAColor)*w*h);
    switch(tex_data->texture_type) {
        case BPP4_UNSWIZZLED:
            swfBITMAP_extract_4bpp(pck, info1, &colors);
            break;
        case BPP8_INDEX_SWIZZLED_BUT_TEXTURE_NOT:
            swfBITMAP_extract_8bpp(pck, info1, &colors, 0);
            break;
        case BPP8_BOTH_SWIZZLED:
            swfBITMAP_extract_8bpp(pck, info1, &colors, 1);
            break;
        default:
            fprintf(stderr, "%.8x, unknown texture type\n",  pckData_get_og_from_ptr(pck, bmp));
            return;
    }
    // hack to make images not look transparent
    for(int i = 0; i < bmp->width * bmp->height; i++) {
        if(colors[i].a != 0)
            colors[i].a = 255;
    }
    stbi_write_png(filepath, w, h, 4, colors, 0);
}

int main(int argc, char* argv[]) {
    PckData pck;
    char* fpath = argv[1];
    pckData_init(&pck, fpath);
    swfFILE* file = (swfFILE*)(pck.actual_data);
    printf("Object count: %d\n", file->objectCount);

    char outdir[1024];
    char input_copy[1024];

    strncpy(input_copy, fpath, sizeof(input_copy));
    input_copy[sizeof(input_copy) - 1] = '\0';
    
    // Remove extension
    char *dot = strrchr(input_copy, '.');
    if (dot)
        *dot = '\0';
    
    snprintf(outdir, sizeof(outdir), "%s", input_copy);
    
    make_directory(outdir);

    for(int obj_p = 1; obj_p < file->objectCount; obj_p++) {
        swfOBJECT_header* info = pckData_get_obj(&pck, obj_p);

        int otIndex = info->objectType;

        switch(info->objectType) {
            case OBJ_BITMAP:
            char image_path[1024];
            snprintf(image_path, sizeof(image_path), "%s/%d.png", outdir, obj_p); 
            printf("Writing image %d\n", obj_p);
            to_png(&pck, (swfBITMAP*)info, image_path);
            break;
        }
    }
    return 0;
}
