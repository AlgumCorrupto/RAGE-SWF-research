#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <string.h>

#pragma pack(push, 1) // All packed struct, don't let compiler align

// just to make things more clear
// when i refer to "character id", "object id" or whatever id,
// i'm referring to its position in the swfOBJECTS array pointed by swfFILE.
// capiche?

// printing swfOBJECTS config
#define PRINT_SPRITE 1
#define PRINT_BITMAP 0
#define PRINT_TEXT 0
#define PRINT_SHAPE 0
#define PRINT_EDITTEXT 0
// all of these to the top were partially reversed in some way,
// bottom ones still a work in progress
#define PRINT_BUTTON 1
#define PRINT_FONT 0

// write bitmap to files, must have PRINT_BITMAP on
#define WRITE_BITMAP 1
#define WRITE_TEXT 1

// clankkka wrote this
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
// clankkka over

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
    uint32_t image_current_node_ptr;
    uint32_t unk3;
    uint8_t pad1[4];
} bmpInfo1;

typedef struct {
    uint32_t unk1; // always 1???
    uint32_t next_image_ptr; //????? it points to another another image, like a linked list
    uint32_t ptr_to_texture; // pointer to the actual texture data
    uint16_t maybe_bits;
    uint16_t width;
    uint16_t height;
    uint8_t pad1[12];
} BitmapLinkedListNode;

typedef struct {
    uint16_t unk1;
    uint16_t unk2;
    uint32_t unk3;
    uint32_t unk4;
    uint32_t unk5;
    uint32_t unk6;
    // padded until address is multiple of 0x40
    //char pad1[];
    //void* data;
} bmpTexture;

typedef struct {
    uint32_t ptr_to_info1; // pointer to something
    // in pixels
    uint16_t width; 
    uint16_t height;
    float inv_width;  // 1/width
    float inv_height; // 1/height
} swfBITMAP;

typedef struct {
    uint8_t b, g, r, a;
} ARGBcolor;

typedef struct {
    float ax, ay, bx, by, cx, cy;
} swfMATRIX;

// i should have used C++ class inheritance
typedef enum {
    TR_END = 0,
    TR_FONT,
    TR_COLOR,
    TR_YOFFSET,
    TR_XOFFSET,
    TR_GLYPHENTRY
} TextRecord_Types;

typedef struct {
    uint16_t type; // value: 1
    uint16_t font_id; // object id of the font
    uint16_t font_size;
} TextRecord_FontConfig;
typedef struct {
    uint16_t type; // value: 2
    ARGBcolor color_rgba;
} TextRecord_Color;
typedef struct {
    uint16_t type; // value: 3 or 4
    int16_t  offset;    
} TextRecord_Offset;

typedef struct {
    uint16_t glyph_index; // index in the font table
    uint16_t glyph_advance; // horizontal distance to the next glyph
} GlyphEntry;

typedef struct {
    uint16_t type; // value: 5
    uint16_t glyph_count;
    // from now on there's a list of glyph entries.
    GlyphEntry entries[];
} TextRecord_GlyphArray; // case 5

typedef struct {
    swfMATRIX matrix;
    uint32_t text_records_ptr;
} swfTEXT;

typedef struct {
    uint16_t entry_count;
    uint16_t pad1; // 0xcdcd
    uint32_t ptr1; // array of pointers that cant be found in the file for some reason
    uint32_t character_array; // pointer to an array containing 16 bit characters apparently, 
    uint32_t ptr2; // i suppose its another array, but not sure of what, sometimes is null.
    // both have the same quantity of entries from the entry_count. 
}swfFONT ;

typedef struct {
    uint8_t pad1[3]; // 0xCDCDCD
    uint8_t frame_count8;  // frame count seems to be duplicated for some reason, maybe one is framerate and other
                           // is the actual frame count, no idea.
    uint32_t frames;
    uint32_t frame_count32;
} swfSPRITE;

typedef struct {
    uint32_t null0; // always null as far as i'm aware of
    uint32_t commands; // pointer to swfCMDs
} swfFRAME;

typedef struct {
    // both of them are composed by fixed point 8.8 RGBA
    ARGBcolor mult_term;
    ARGBcolor add_term;
} swfCXFORMWITHAPLHA;

typedef struct {
    uint32_t vtable;
    uint8_t cmd_type;
    uint8_t flags; // bit 0 defines if it has scale, bit 1 if it has rotation
    uint8_t useless[2];
    uint32_t next_CMD; // next command in the linked list
} swfCMDHeader;

typedef struct {
    // insert swfCMD_Header here
    uint16_t depth; 
    uint16_t character_id; // if character is 0xFFFF, that means a eaew character needs to be created
    uint32_t packed_matrix_ptr; 
    uint32_t color_xform_ptr;
} swfCMD_placeObject2;

typedef struct {
    uint32_t unk1;
    uint32_t action_list_ptr;
} codeWrapper;

typedef struct {
    // insert swfCMD_Header here
    uint16_t depth; 
    uint16_t character_id; // if character is 0xFFFF, that means a eaew character needs to be created
    uint32_t packed_matrix_ptr; 
    uint32_t color_xform_ptr;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t code_wrapper_ptr;
    uint32_t name_ptr; // pointer to a standard ascii string that has the name of the movie clip
} swfCMD_clipEvent;

// fillstyle commands sheet:
// 0: end of commands
// 1: draw lines/strokes
// 2: fill
// 3: unknown
// 4: fillstyle change (color or texture)
// 5: strokestyle change
typedef enum {
    SH_END = 0,
    SH_DRAW_STROKES,
    SH_DRAW_FILL,
    SH_UNKNOWN, // not mentioned in the decompiled code
    SH_FILLSTYLE_CHANGE,
    SH_STROKESTYLE_CHANGE,
} swfSHAPE_opcodes;

typedef struct {
    uint16_t x;
    uint16_t y;
} swfPoint;

typedef struct {
    swfMATRIX gradient_matrix; // some matrix of floats
    uint16_t stops_count; // quantity of colors in the gradients
    char pad1[0x2];
    uint32_t ratio_ptr; // pointer to an array of uint8_t
    uint32_t colors_ptr;// pointer to a rgba array
} swfGRADIENT;

typedef struct {
    uint8_t unk1; // no correlation to anything, maybe flags? not sure
    char tak_marker[3]; // for some reason there's a random ascii sequence 'tak'
    ARGBcolor color_data; // rgba
    swfMATRIX bitmap_matrix; // it's an identity matrix unless bitmap pointer points to something.
    uint32_t bitmap_pointer; // if has a bitmap, this is not null
    uint32_t gradient_pointer; 
} swfSHAPE_FillStyle_data;

typedef struct {
    uint16_t opcode; // 4
    uint16_t style_index;
} swfSHAPE_FillStyle;

typedef struct {
    // opcode 1 for draw outline
    // opcode 2 for draw filled
    uint16_t opcode;
    uint16_t vertex_count;
    // from now on an array of points.
    swfPoint points[];
} swfSHAPE_Polygon;

typedef struct {
    uint16_t unk1;
    uint16_t unk2;
    ARGBcolor color;
} swfSHAPE_StrokeStyle_data;

typedef struct {
    uint16_t opcode; // 5
    uint16_t style_index;
} swfSHAPE_StrokeStyle;

typedef struct {
    uint32_t fill_style_table;
    uint32_t stroke_style_table;
    uint32_t display_list_ptr;
} swfSHAPE;

typedef enum {
    AL_LEFT = 0,
    AL_RIGHT,
    AL_CENTER,
    AL_JUSTIFY,
} swfEDITTEXT_alignment;

typedef struct {
    // zero idea if this is right
    uint32_t initial_text;                
    uint32_t lookup_key;

    uint16_t has_initial_text;
    int16_t leading;

    // confident on this
    ARGBcolor color;

    // also confident on this
    uint16_t font_index;
    uint16_t font_size;

    // also confident
    int32_t width;
    int32_t height;

    // maybe?
    int32_t xoffset;
    int32_t yoffset;

    // 0 = left
    // 1 = right
    // 2 = center
    // 3 = justify
    uint8_t alignment;
} swfEDITTEXT;

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

// useless function
uint32_t getAbsoluteAddrFromOgAddress(uint32_t base, uint32_t ogAddr) {
    uint32_t relative = getRelAddrFromOgAddress(ogAddr);
    return base + relative;
}

ARGBcolor add_color(ARGBcolor c1, ARGBcolor c2) {
    return (ARGBcolor){
        .a = c1.a + c2.a,
        .b = c1.b + c2.b,
        .g = c1.g + c2.g,
        .r = c1.r + c2.r,
    };
}

ARGBcolor mult_color(ARGBcolor c1, ARGBcolor c2) {
    return (ARGBcolor){
        .a = c1.a * c2.a,
        .b = c1.b * c2.b,
        .g = c1.g * c2.g,
        .r = c1.r * c2.r,
    };
}

ARGBcolor xform_color(ARGBcolor c, swfCXFORMWITHAPLHA x) {
    return add_color(x.add_term, mult_color(c, x.mult_term));
}

void print_color(ARGBcolor c) {
    printf("\x1b[38;2;%d;%d;%dm", c.r, c.g, c.b);
    printf("#%.2X%.2X%.2X%.2X", c.r, c.g, c.b, c.a);
    printf("\x1b[0m\n");
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
            switch(cmd->cmd_type) {
                case 0:
                    swfCMD_placeObject2* place_o = (swfCMD_placeObject2*)(cmd + 1);
                    uint32_t* pointed = (uint32_t*)getPtrFromOgAddress(place_o->packed_matrix_ptr);
                    printf("character: %d\n", place_o->character_id); // if character is 0xFFFF, that means a new character needs to be created
                    //if(*pointed != 0) {
                    //    printf("VALUE LIL WEIRD\n0x%.8x\n", *pointed);
                    //}
                    if(place_o->color_xform_ptr != 0) {
                        swfCXFORMWITHAPLHA* xform = (swfCXFORMWITHAPLHA*)getPtrFromOgAddress(place_o->color_xform_ptr);
                        print_color(add_color(xform->add_term, xform->mult_term));
                    } 
                    break;
                case 1:
                    swfCMD_clipEvent* clip_e = (swfCMD_clipEvent*)(cmd + 1);
                    if(clip_e->name_ptr != 0 ) {
                        char* name = (char*)getPtrFromOgAddress(clip_e->name_ptr); 
                        printf("%s\n", name);
                   }
                default:
                    break;
            }
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

    if(PRINT_SPRITE) {
        swfFRAME *file_frame = getPtrFromOgAddress(si->frames);
        printf("Reading background frames...\n");
        list_frames(file_frame, si->frame_count);
    }

    char outdir[1024];
    char input_copy[1024];

    // if i care to write the bitmap,
    // i create a folder
    // clankkka wrote this
    strncpy(input_copy, fileInputPath, sizeof(input_copy));
    input_copy[sizeof(input_copy) - 1] = '\0';
    
    // Remove extension
    char *dot = strrchr(input_copy, '.');
    if (dot)
        *dot = '\0';
    
    snprintf(outdir, sizeof(outdir), "%s", input_copy);
    
    mkdir(outdir, 0755);
    // clankkka over

    char txt_name[256];
    sprintf(txt_name, "./%s/strings.txt", outdir);
    FILE* txt_file = fopen(txt_name, "w");
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
        switch (oti->objectType) {
        case 1: // swfSHAPE
            if(!PRINT_SHAPE) break;

            swfSHAPE* shape = (swfSHAPE*)(oti + 1);
            printf("Display list localized at 0x%.8x (0x%.8x)\n", shape->display_list_ptr, getRelAddrFromOgAddress(shape->display_list_ptr));
            printf("Display list data localized at 0x%.8x (0x%.8x)\n", shape->fill_style_table, getRelAddrFromOgAddress(shape->fill_style_table));
            uint8_t* value_pointed = (uint8_t*)getPtrFromOgAddress(shape->stroke_style_table); 
            uint16_t* opcode = (uint16_t*) getPtrFromOgAddress(shape->display_list_ptr);
            uint8_t ended_s = 0;
            while(!ended_s) {
                switch(*opcode) {
                case SH_END: // 0
                    ended_s = 1;
                    break;
                case SH_DRAW_STROKES: // 1
                case SH_DRAW_FILL: // 2
                    swfSHAPE_Polygon* poly =  (swfSHAPE_Polygon*)opcode;
                    //printf("has %d points\n", poly->vertex_count);
                    opcode = (uint16_t *)&poly->points[poly->vertex_count];
                    break;
                //case 3 is is never processed or mentioned in the decompiled code as far as i'm concerned
                case SH_FILLSTYLE_CHANGE:  // 4
                    swfSHAPE_FillStyle* fillstyle = (swfSHAPE_FillStyle*)opcode;
                    //printf("fillstyle index is %d\n", fillstyle->style_index);
                    swfSHAPE_FillStyle_data* frecord = (swfSHAPE_FillStyle_data*)getPtrFromOgAddress(
                        shape->fill_style_table + sizeof(swfSHAPE_FillStyle_data) * (fillstyle->style_index-1)
                    );
                    if(frecord->bitmap_pointer != 0) {
                        printf("the bitmap pointer points to to 0x%.8x (0x%.8x)\n", frecord->bitmap_pointer, getRelAddrFromOgAddress(frecord->bitmap_pointer));
                    } else {
                        print_color(frecord->color_data);
                    }
                    //if(memcmp(record->tak_marker, "tak", 3)) {
                    //    printf("=============\nTHIS ONE DOES NOT HAVE 'tak'\nvalue: %.2x %.2x %.2x\n", record->tak_marker[0], record->tak_marker[1], record->tak_marker[2]);
                    //}
                    if(frecord->gradient_pointer != 0) {
                        swfGRADIENT* gradient = (swfGRADIENT*)getPtrFromOgAddress(frecord->gradient_pointer);
                        printf("printing gradients\n");
                        for(int g_i = 0; g_i < gradient->stops_count; g_i++) {
                            ARGBcolor* gcolor = (ARGBcolor*)getPtrFromOgAddress(gradient->colors_ptr + sizeof(ARGBcolor) * g_i);
                            print_color(*gcolor);
                        }
                    }
                    opcode = (uint16_t*)(((swfSHAPE_FillStyle*) opcode) + 1);
                    break;
                case SH_STROKESTYLE_CHANGE: // 5
                    swfSHAPE_StrokeStyle* strokestyle = (swfSHAPE_StrokeStyle*)opcode;
                    swfSHAPE_StrokeStyle_data* srecord = (swfSHAPE_StrokeStyle_data*)getPtrFromOgAddress(
                        shape->stroke_style_table + sizeof(swfSHAPE_StrokeStyle_data) * (strokestyle->style_index - 1)
                    ); 
                    printf("style index stored is  %d\n", strokestyle->style_index);
                    opcode = (uint16_t*)(((swfSHAPE_StrokeStyle*) opcode) + 1);
                    break;
                default:
                    printf("opcode %d\n", *opcode);
                    printf("unknown\n");
                    ended_s = 1;
                }
            }
            break;
        case 2: // swfSPRITE
            if(!PRINT_SPRITE) break;

            swfSPRITE* sprite = (swfSPRITE*)(oti + 1);

            swfFRAME* frame = getPtrFromOgAddress(sprite->frames);
            list_frames(frame, sprite->frame_count32);
            break;
        case 4: // swfBITMAP
            if(!PRINT_BITMAP) break;

            swfBITMAP* bitmap = (swfBITMAP*)(oti + 1);
            printf("w: %d, h: %d\n", bitmap->width, bitmap->height);
            printf("info 1... 0x%.8x\n", bitmap->ptr_to_info1);
            bmpInfo1* info1 = getPtrFromOgAddress(bitmap->ptr_to_info1);
            printf("info 2... 0x%.8x\n", info1->image_current_node_ptr);
            BitmapLinkedListNode* info2 = getPtrFromOgAddress(info1->image_current_node_ptr);
            printf("info 3... 0x%.8x\n", info2->ptr_to_texture);
            bmpTexture* info3 = getPtrFromOgAddress(info2->ptr_to_texture);
            uint32_t end = info2->ptr_to_texture + sizeof(bmpTexture);
            //printf("%.8x end", end)
            // getting the closest biggest address multiple of 0x80
            uint32_t multiple = 0x80;
            uint32_t offset = (multiple - (end % multiple)) % multiple;
            uint32_t data_address = offset + end;
            uint8_t* value = getPtrFromOgAddress(data_address);
            // I'm not sure what is going on but few images seems to be corrupted.
            // maybe its the gpu swizzling that clankers have been talking about?
            if(*value == 0xCD) {
                printf("=================\nWOOOOOPS LOOKS LIKE YOU ARE FUCKING WRONG\n==============\n");
            }
            while(*value == 0xCD) {
                ++value;
                data_address++;
            }

            uint32_t image_end = data_address + (bitmap->width * bitmap->height);
            printf("Image data localized at 0x%.8x (begin), 0x%.8x (end)\n", data_address, image_end);

            // if i care to write the bitmap, for each bitmap found, write it to disk
            if(WRITE_BITMAP) {
                // clankkka wrote this
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
                // clankkka over
            }

            break;
            case 5: //swfFONT
                break;
            case 6: //swfTEXT
                if(!PRINT_TEXT) break;
                swfTEXT* text_obj = (swfTEXT*)(oti + 1);
                printf("text record at %.8x\n", text_obj->text_records_ptr);

                uint16_t* text_record_type = getPtrFromOgAddress(text_obj->text_records_ptr);
                uint8_t ended_t = 0;

                // if you not sure what is the deal with this fucked up looking expressions,
                // its just pointer math.
                // https://youtu.be/95M6V3mZgrI?si=JDwkVJiriQ9Pv6VY 
                swfFONT* current_font = 0;

                while (!ended_t) {
                    switch (*text_record_type) {
                    case TR_END:
                        /* code */
                        ended_t = 1;
                        break;
                    case TR_COLOR:
                        TextRecord_Color* tr_color = (TextRecord_Color*)(text_record_type);
                        print_color(tr_color->color_rgba);
                        text_record_type = (uint16_t*)(((TextRecord_Color*)text_record_type) + 1);
                        break;
                    case TR_FONT:
                        TextRecord_FontConfig* font = (TextRecord_FontConfig*)text_record_type;
                        uint32_t font_address = si->pointToObjectPtrList + sizeof(uint32_t) * font->font_id;
                        uint32_t* actual_pointer_obj =  (uint32_t*)getPtrFromOgAddress(font_address);
                        current_font = (swfFONT*)((PckSwfObjectTypeInfo*)(getPtrFromOgAddress(*actual_pointer_obj)) + 1);
                        text_record_type = (uint16_t*)(((TextRecord_FontConfig*)text_record_type) + 1);
                        break;
                    case TR_XOFFSET:
                    case TR_YOFFSET:
                        //printf("text offset\n");
                        text_record_type = (uint16_t*)(((TextRecord_Offset*)text_record_type) + 1);
                        break;
                    case TR_GLYPHENTRY: // i'm caring about displaying the text
                        TextRecord_GlyphArray* glyph_array = (TextRecord_GlyphArray*)text_record_type;
                        GlyphEntry* entry = (GlyphEntry*)(glyph_array + 1);
                        uint16_t* characters = getPtrFromOgAddress(current_font->character_array);
                        //printf("text glyph\n");
                        //printf("font characters at 0x%.8x\n", current_font->character_array);
                        //printf("printing the text...\n");
                        if(WRITE_TEXT) {
                            for(size_t f_i = 0; f_i < glyph_array->glyph_count; f_i++) {
                                fprintf(txt_file, "%c", characters[entry->glyph_index]);
                                ++entry;
                            }
                            fprintf(txt_file, "\n");
                        }

                        text_record_type = (uint16_t*)&glyph_array->entries[glyph_array->glyph_count];
                        break;
                    default:
                        printf("unknown text record\n");
                        ended_t = 1;
                        break;
                }
            }
            break;
        case 7: //swfEDITTEXT
            if(!PRINT_EDITTEXT) break;

            swfEDITTEXT* editt = (swfEDITTEXT*)(oti + 1);
            print_color(editt->color);
            break;
        default:
            break;
        }
    }

    fclose(txt_file);
    free(pckData);
}
