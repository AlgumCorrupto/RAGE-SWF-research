#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <libgen.h>
#include <string.h>

#include "../flashpck.h"

#ifdef _WIN32
    #include <direct.h>
    #define make_directory(path) _mkdir(path)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define make_directory(path) mkdir(path, 0777)
#endif

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
            printf("Writing image %d\n", info->objectType);
            swfBITMAP_extract(&pck, (swfBITMAP*)info, image_path);
            break;
        }
    }
    return 0;
}
