#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <string.h>

#pragma pack(push, 1) // All packed struct, don't let compiler align


#define WRITE_BITMAP 0


// remove later
typedef struct {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwFourCC;
    uint32_t dwRGBBitCount;
    uint32_t dwRBitMask;
    uint32_t dwGBitMask;
    uint32_t dwBBitMask;
    uint32_t dwABitMask;
} DDS_PIXELFORMAT;

typedef struct {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwHeight;
    uint32_t dwWidth;
    uint32_t dwPitchOrLinearSize;
    uint32_t dwDepth;
    uint32_t dwMipMapCount;
    uint32_t dwReserved1[11];

    DDS_PIXELFORMAT ddspf;

    uint32_t dwCaps;
    uint32_t dwCaps2;
    uint32_t dwCaps3;
    uint32_t dwCaps4;
    uint32_t dwReserved2;
} DDS_HEADER;

#define MAKEFOURCC(a,b,c,d) \
    ((uint32_t)(a) | ((uint32_t)(b)<<8) | ((uint32_t)(c)<<16) | ((uint32_t)(d)<<24))

void write_dds_header(FILE *f, uint32_t width, uint32_t height, uint32_t linearSize)
{
    fwrite("DDS ", 1, 4, f);

    DDS_HEADER h;
    memset(&h, 0, sizeof(h));

    h.dwSize = 124;
    h.dwFlags =
        0x1 |      // DDSD_CAPS
        0x2 |      // DDSD_HEIGHT
        0x4 |      // DDSD_WIDTH
        0x80000 |  // DDSD_LINEARSIZE
        0x1000;    // DDSD_PIXELFORMAT

    h.dwHeight = height;
    h.dwWidth = width;
    h.dwPitchOrLinearSize = linearSize;

    h.ddspf.dwSize = 32;
    h.ddspf.dwFlags = 0x4; // DDPF_FOURCC
    h.ddspf.dwFourCC = MAKEFOURCC('D','X','T','5');

    h.dwCaps = 0x1000; // DDSCAPS_TEXTURE

    fwrite(&h, sizeof(h), 1, f);
}
// remove later

// not sure what this struct
typedef struct {
    uint8_t num;
    uint8_t frag;
} SwfFixed8_8;

typedef struct {
    uint32_t baseAddress;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t dataSize;
} PckFileHeader;

typedef struct {
    uint32_t pointsToSwfVtable; // Points to swfFILE
    uint8_t objectType; // swfOBJECT type atau 0 pada awal
    char padding1[3]; // 0xcdcdcd padding
} PckSwfObjectTypeInfo;

typedef struct {
    uint16_t unk1; // Selalu 1
    char padding1[2]; // 0xcdcd padding
    uint32_t frames; // 0x30 at data
    uint32_t pointToObjectPtrList; // Begin pointer list
    uint8_t framePerSecond; // Selalu 6, fps?
    char padding2[3]; // 0xcdcdcd padding
    uint8_t unk3[6]; // all 0
    uint16_t maybeWidth; // maybe width in twips
    uint8_t unk4[6]; // all 0
    uint16_t unk5; // selalu 0x43f0
    uint16_t maybeHeight; // maybe height in twips
    uint16_t objectCount; // jumlah objek di pointer + objek ini
    uint32_t frame_count; // quantity of frames??? who fucking knows
} swfFILE;

// this is probably just a swf frame??
typedef struct {
    uint32_t backgroundRGBA; // Probably background in RGBA?
    uint32_t pointsToClass_swfACTION_DoAction; // initial script executed by flash usually at 0x40 but not always, controllers.xbck at 0x2564
    char padding[8]; // usual 0xcd padding
} PckSwfBackgroundInfo;

typedef struct {
    uint32_t bytecode; 
} swfDoAction;

typedef struct {
    uint16_t unk1; // 01 00
    // then you have the AVM1 bytecode, i'm not gonna bother representing in the struct
    // remember that avm1 bytecode functions similar to a c string,
    // that there's a specific "instruction" that marks the end of the code.
    // shoutout to bruno for making imhex pattern code that parses the AVM1 bytecode
} AVM1Bytecode;

// TODO: swfBITMAP
// i've discovered with the help of this wonderful tool called texture finder that
// at least in the xbox version, images are stored in the DXT5 format.

// taken from theikus go implementation 
typedef struct {
    uint32_t unk1[2];

    uint16_t width; 
    uint16_t height; 
    uint32_t data_size; // W x H x 4
    uint32_t unk2;
    uint32_t ptr_to_info2;
    uint32_t unk3;
    uint8_t pad1[4];
} bmpInfo1;


typedef struct {
    uint32_t unk1;
    uint32_t ptr1;
    uint32_t ptr_to_info3;
    uint16_t maybe_bits;
    uint16_t width;
    uint16_t height;
    uint8_t pad1[12];
} bmpInfo2;

typedef struct {
    uint16_t unk1;
    uint16_t unk2;
    uint32_t unk3;
    uint32_t unk4;
    uint32_t unk5;
    uint32_t unk6;
    // padded until address is multiple of 0x40, not 44
    //char pad1[44];
    //void* data;
} bmpInfo3;

typedef struct {
    uint32_t ptr_to_info1; // pointer to something
    // in pixels
    uint16_t width; 
    uint16_t height;
    float inv_width;  // 1/width
    float inv_height; // 1/height
} swfBITMAP;


typedef struct {
    uint8_t pad1[3]; // 0xCDCDCD
    uint8_t frame_count8;  // frame count seems to be duplicated for some reason, maybe one is framerate and other
                           // is the actual frame count, no idea.
    uint32_t frames;
    uint32_t frame_count32;
} swfSPRITE;

typedef struct {
    uint32_t null0;
    uint32_t commands; // pointer to swfCMDs
} swfFRAME;

typedef struct {
    uint32_t vtable;
    uint8_t cmd_type;
    uint8_t useless[3];
    uint32_t next_CMD;

} swfCMDHeader;

typedef struct {
    uint16_t unk1;
    uint16_t character_id;
    // from now on i'm not sure WHAT THE FUCK this data means
    uint32_t ptr1; // has some bullshit in the beginning
                   // then some action script bytecode


} swfCMD_placeObject2;


typedef struct {
    uint16_t unk1;
    uint16_t character_id;
    uint32_t ptr1; // has some bullshit in the beginning
                        // then some action script bytecode

    // i DONT KNOW
    //uint32_t null_or_ptr1;
    //uint32_t unk3; 
    //uint32_t unk4;
    //uint32_t ptr2ptr2bytecode; // unknown pointer for now
} swfCMD_clipEvent;

typedef struct {
    float matrix[4]; // i suppose this is a 4x4 matrix
    uint32_t null1[2];
    uint32_t fill_color_ptr;
} swfTEXT;

// fillstyle commands sheet:
// 0: end of commands
// 1: draw lines/strokes
// 2: fill
// 3:
// 4: style change (color or texture)
typedef enum {
    END = 0,
    DRAW_STROKES,
    DRAW_FILL,
    SKIPPED,
    COLOR_CHANGE,
} swfSHAPE_commands;

typedef enum {
    SOLID_COLOR = 0,
    LINEAR_GRADIENT = 0x10,
    RADIAL_GRADIENT = 0x12,
    // it means bitmap image
    // but i'm not sure what is the variation
    BITMAP_IMAGE0 = 0x40,
    BITMAP_IMAGE1 = 0x41,
    BITMAP_IMAGE2 = 0x42,
    BITMAP_IMAGE3 = 0x43,
} FillType;

typedef struct {
    uint8_t fill_type; // from the FillType enum
    char tak_magic[3]; // for some reason there's a random ascii sequence 'tak'
    uint32_t color_data; // rgba
    float uv_matrix[4]; // gradient/texture transform
    float translation[2];
} FillCommand;

typedef struct {
    uint32_t style_data;
    uint32_t linestyle_array; // maybe?
    uint32_t style_commands;
} swfSHAPE;

#pragma pack(pop) // End packed struct

char swfObjectTypesString[10][16] = {
    "header", "swfSHAPE", "swfSPRITE", "swfBUTTON", "swfBITMAP", "swfFONT", "swfTEXT", "swfEDITTEXT", "swfSOUND", "swfMORPHSHAPE"
};

char swfCmdTypesString[4][32] = {
    "swfPlaceObject2", "swfClipEvent", "swfDoInitAction", "swfDoAction"
};

static uint32_t savedOgBaseAddress = 0;
static void *savedCurBaseAddress;

void setOriginalBaseAddress(uint32_t baseAddr)
{
    savedOgBaseAddress = baseAddr;
}

void setCurrentBaseAddress(void *curBaseAddr)
{
    savedCurBaseAddress = curBaseAddr;
}

void *getPtrFromOgAddress(uint32_t ogAddr)
{
    return savedCurBaseAddress + (ogAddr - savedOgBaseAddress);
}

uint32_t getOgAddressFromPointer(size_t pointer) {
    return (uint32_t)(pointer - (size_t)savedCurBaseAddress);
}

uint32_t getRelAddrFromOgAddress(uint32_t ogAddr)
{
    return ogAddr - savedOgBaseAddress;
}

uint32_t getAbsoluteAddrFromOgAddress(uint32_t base, uint32_t ogAddr) {
    uint32_t relative = getRelAddrFromOgAddress(ogAddr);
    return base + relative;
}

void list_frames(swfFRAME* frame, uint32_t count) {
    for(int f_i = 0; f_i < count; f_i++) {
        if(frame->commands == 0) continue; 

        // uint32_t frame_display = getRelAddrFromOgAddress(sprite->frames);

        uint32_t cmd_relative = getRelAddrFromOgAddress(frame->commands);

        printf("swfFRAME: %d\n", f_i);
        
        swfCMDHeader *cmd = getPtrFromOgAddress(frame->commands);
        uint32_t old = frame->commands;
        int count = 0;
        do {
            printf("swfCMD %d: 0x%.8x (0x%.8x), of type %d %s\n", count++, old, cmd_relative, cmd->cmd_type, swfCmdTypesString[cmd->cmd_type]);
            if(cmd->next_CMD == 0) break;
            cmd_relative = getRelAddrFromOgAddress(cmd->next_CMD);
            old = cmd->next_CMD;
            cmd = getPtrFromOgAddress(cmd->next_CMD);
        } while(1);

        printf("\n");
        ++frame;
    }
}

int main(int argc, char* argv[]) {
    void *pckData;
    PckFileHeader fileHeader;
    char *fileInputPath = argv[1];
    FILE *fin = fopen(fileInputPath, "rb");

    fseek(fin, 0, SEEK_END);
    uint32_t fileSize = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    fread(&fileHeader, sizeof(PckFileHeader), 1, fin);
    printf("File %s is %d bytes with 0x%.8x base\n", fileInputPath, fileSize, fileHeader.baseAddress);
    uint32_t dataSize = fileSize - 128;
    if (dataSize != fileHeader.dataSize) {
        printf("Invalid data size %d != %d\n", dataSize, fileHeader.dataSize);
        return 1;
    }
    setOriginalBaseAddress(fileHeader.baseAddress);

    // Load data and close file
    fseek(fin, 128, SEEK_SET);
    pckData = malloc(dataSize);
    fread(pckData, dataSize, 1, fin);
    fclose(fin);
    setCurrentBaseAddress(pckData);
    printf("Data loaded at 0x%.16x\n", pckData);

    // Header info next to 
    swfFILE *si = pckData + sizeof(PckSwfObjectTypeInfo);
    int widthPx = si->maybeWidth / 20;
    int heightPx = si->maybeHeight / 20;
    int objectCount = si->objectCount;
    printf("SWF header info: W=%d H=%d fps=%d Objects=%d\n", widthPx, heightPx, si->framePerSecond, objectCount);

    uint32_t object_base = savedOgBaseAddress + 0x80;

    // Background
    swfFRAME *file_frame = getPtrFromOgAddress(si->frames);
    printf("Reading background frames...\n");
    list_frames(file_frame, si->frame_count);

    char outdir[1024];
    char input_copy[1024];

    // if i care to write the bitmap,
    // i create a folder
    if(WRITE_BITMAP) {
    
        strncpy(input_copy, fileInputPath, sizeof(input_copy));
        input_copy[sizeof(input_copy) - 1] = '\0';
    
        // Remove extension
        char *dot = strrchr(input_copy, '.');
        if (dot)
            *dot = '\0';
    
        snprintf(outdir, sizeof(outdir), "%s", input_copy);
    
        mkdir(outdir, 0755);
    }

    printf("Where the object list is located: 0x%.8x\n", si->pointToObjectPtrList);
    // Object list
    uint32_t *ol = getPtrFromOgAddress(si->pointToObjectPtrList);
    // Index 0 is skipped because header object already read and this first entry always null
    for (int i = 1; i < objectCount; i++) {
        ++ol;
        PckSwfObjectTypeInfo *oti = getPtrFromOgAddress(*ol);
        uint32_t otiRelAddr = getRelAddrFromOgAddress(*ol);
        int otIndex = oti->objectType;
        if (otIndex >= 0 && otIndex <= 9) {
            printf("SWF object %d located at 0x%.8x (0x%.8x), type %d (%s)\n", i, *ol, otiRelAddr, otIndex, swfObjectTypesString[otIndex]);
        } else {
            printf("Invalid object type index %d at %d\n", otIndex, i);
            return 1;
        }
        switch (oti->objectType){
        case 2: // swfSPRITE
            swfSPRITE* sprite = (swfSPRITE*)(oti + 1);

            swfFRAME* frame = getPtrFromOgAddress(sprite->frames);
            list_frames(frame, sprite->frame_count32);
            break;
        case 4: // swfBITMAP
            swfBITMAP* bitmap = (swfBITMAP*)(oti + 1);
            printf("w: %d, h: %d\n", bitmap->width, bitmap->height);
            printf("info 1... 0x%.8x\n", bitmap->ptr_to_info1);
            bmpInfo1* info1 = getPtrFromOgAddress(bitmap->ptr_to_info1);
            printf("info 2... 0x%.8x\n", info1->ptr_to_info2);
            bmpInfo2* info2 = getPtrFromOgAddress(info1->ptr_to_info2);
            printf("info 3... 0x%.8x\n", info2->ptr_to_info3);
            bmpInfo3* info3 = getPtrFromOgAddress(info2->ptr_to_info3);
            uint32_t end = info2->ptr_to_info3 + sizeof(bmpInfo3);
            //printf("%.8x end", end)
            // getting the closest biggest address multiple of 0x80
            uint32_t multiple = 0x80;
            uint32_t offset = (multiple - (end % multiple)) % multiple;
            uint32_t data_address = offset + end;
            uint8_t* value = getPtrFromOgAddress(data_address);
            uint32_t image_end = data_address + (bitmap->width * bitmap->height);
            // I'm not sure what is going on but few images seems to be corrupted.
            printf("Image data localized at 0x%.8x (begin), 0x%.8x (end)\n", data_address, image_end);
            if(*value == 0xCD) {
                printf("=================\nWOOOOOPS LOOKS LIKE YOU ARE FUCKING WRONG\n==============\n");
            }

            // if i care to write the bitmap, for each bitmap found, write it to disk
            if(WRITE_BITMAP) {
                uint32_t compressed_size = info1->width*info1->height;   // Use this instead of width*height
                uint8_t *image_data = getPtrFromOgAddress(data_address);

                // Write raw DXT5 blocks
                char raw_name[1024];
                snprintf(raw_name, sizeof(raw_name), "%s/%d.raw", outdir, i);

                char dds_name[1024];
                snprintf(dds_name, sizeof(dds_name), "%s/%d.dds", outdir, i);

                FILE *dds = fopen(dds_name, "wb");

                write_dds_header(dds,
                                 bitmap->width,
                                 bitmap->height,
                                 compressed_size);
            
                fwrite(image_data, 1, compressed_size, dds);
                fclose(dds);

                // Convert to PNG
                char png_name[1024];
                snprintf(png_name, sizeof(png_name), "%s/%d.png", outdir, i);

                char command[4096];
                snprintf(command, sizeof(command),
                    "ffmpeg -loglevel error -y "
                    "-i \"%s\" "
                    "\"%s\"",
                    dds_name,
                    png_name);

                system(command);

                // Remove temporary raw file
                remove(dds_name);
            }
        default:
            break;
        }
    }

    free(pckData);
}
