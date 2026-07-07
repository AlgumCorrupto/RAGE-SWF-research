#pragma once

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// this is a utility
// structure that you will be using
// 90% of the time when using the API
// for navigating around the file
// with the pckData* functions
// this is not stored in the actual file
// just an utility struct
typedef struct {
    uint32_t og_base_address;
    uint32_t data_size;
    void* actual_data;
} PckData;


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
#pragma pack(push, 1) // All packed struct, don't let compiler align


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

// so every structure inherited from
// swfOBJECT has this same exact first
// fields defined in swfOBJECT_header
typedef struct {
    uint8_t objectType; // swfOBJECT type atau 0 pada awal
    char padding1[3]; // 0xcdcdcd padding
    uint32_t pointsToSwfVtable; // Points to swfFILE
} swfOBJECT_header;

// the "ROOT of everything in the file"
// from this object you can get to anywere else
// after the header and the 0x80 bytes padding
// this is always the first structure that
// appears in the xbox version 
// after 0x80 bytes padding of the header
typedef struct {
    swfOBJECT_header header;
    uint16_t frame_count; // Selalu 1
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
    uint32_t unk; // quantity of frames_ptr??? who fucking knows
} swfFILE;

// swfBITMAP structs

typedef struct {
    uint8_t r, g, b, a;
} RGBAColor;


typedef struct {
    uint16_t unk3; // always 1
    uint16_t color_count;
    uint8_t unk2[0x10 - 0x4];
    RGBAColor indexed_colors[];
} ps2IndexedColors;

// implemented with help from ZNX
// this data is the middlemen between the
// swfBITMAP and the actual ps2Texture,
// it has a bunch of unknown fields and
// it holds a pointer to the ps2texture data
// and the index colors array
typedef struct {
    uint8_t  unk1[0x80];

    uint8_t  unk2[0x8];
    uint16_t width;
    uint16_t height;
    uint16_t unk3;
    uint16_t unk4;

    // 0x90
    uint32_t indexed_colors_ptr;

    // 0x94
    uint32_t ptr_to_tex_data;

    // 0x98
    uint8_t  unk5[0x08];

    // 0xA0
    uint16_t unk6;

    // 0xA2
    uint16_t color_count;          // 1 = 16 colors, 4 = 256 colors
    uint8_t unk7;
    uint8_t pad1[3];
    uint16_t swizzled; // sems to be 4 if texture is swizzled
    uint16_t unk8;
    uint8_t pad2[4];
} bmpInfo1;

typedef enum {
    BPP4_UNSWIZZLED = 0x6,
    BPP8_INDEX_SWIZZLED_BUT_TEXTURE_NOT = 0x5,
    BPP8_BOTH_SWIZZLED = 0x1,
} ps2TextureType;

typedef struct {
    uint32_t unk1; // always 1???
    uint32_t next_image_ptr; //????? it points to another another image, like a linked list
    uint32_t ptr_to_texture; // pointer to the actual texture data
    uint8_t pad1[0x4 + 0x8 + 0x10]; // 0xCDCDCDCD...
    uint16_t unk2;
    uint16_t unk3;
    uint16_t width;
    uint16_t height;
    uint8_t texture_type; // ps2TextureType enum
    uint8_t unk4;
    uint8_t unk5;
    uint8_t flag1;
    uint16_t unk6;
    uint8_t pad2[0x4];
    uint8_t unk7[0x8];
} ps2Texture;

// a swf bitmap as you may have guessed is an object that stores an image.
// actually it stores a pointer (bmpInfo1) to a pointer (ps2Texture) to a pointer that only then
// contains the image data.
typedef struct {

    swfOBJECT_header header;
    uint32_t ptr_to_info1; // pointer to something
    // in pixels
    uint16_t width; 
    uint16_t height;
    float inv_width;  // 1/width
    float inv_height; // 1/height
} swfBITMAP;

typedef struct {
    float ax, ay, bx, by, cx, cy;
} swfMATRIX;

// swfTEXT structs

// the text record types
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
    RGBAColor color_rgba;
} TextRecord_Color;
typedef struct {
    uint16_t type; // value: 3 or 4
    int16_t  offset;    
} TextRecord_Offset;
// okay this one is interesting
// this struct is supposed to store the text that goes in the screen
// but it is stored in a weird way, its not ASCII or any other encoding
// that we are used to, it actually stores the position of the desired character
// in relation to the font table, if the character 'a' needs to be stored but font 
// stores the character 'A' in the position 0
// the value 0 is stored in glyph_index not whatever is its character code in the ascii table.
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

// this represents a static text
// a text does not only have a string, its much more convoluted than that
// it stores this text records ptr, contains instructions to change its
// style like color, layout and text content, its basically what some
// apps like microsoft word or godot calls "RichText"
typedef struct {
    swfOBJECT_header header;
    swfMATRIX matrix;
    uint32_t text_records_ptr;
} swfTEXT;

// swfFONT

// this represents a font, it can be a vector font or a bitmap font, not fully understood by me yet - AlgumCorrupto
typedef struct {
    swfOBJECT_header header;
    uint16_t entry_count;
    uint16_t pad1; // 0xcdcd
    uint32_t ptr1; // array of pointers that cant be found in the file for some reason
    uint32_t character_array; // pointer to an array containing 16 bit characters apparently, 
    uint32_t ptr2; // i suppose its another array, but not sure of what, sometimes is null.
    // both have the same quantity of entries from the entry_count. 
}swfFONT ;


// this does NOT store an image
// as a matter of fact sprite is the equivalent of a movie clip
// you know, these things with timeline
typedef struct {
    swfOBJECT_header header;
    uint8_t pad1[3]; // 0xCDCDCD
    uint8_t frame_count8;  // frame count seems to be duplicated for some reason, maybe one is framerate and other
                           // is the actual frame count, no idea.
    uint32_t frames_ptr;
    uint32_t frame_count32;
} swfSPRITE;

// a swfSPRITE stores an array of frames, a frame is well,
// a frame of the timeline, commands is actually a linked list of commands
// to be realized in that frame.
typedef struct {
    uint32_t null0; // always null as far as i'm aware of, maybe the length of the frame?
    uint32_t commands; // pointer to swfCMDs
} swfFRAME;

// this one is used in swfPlaceObject2 and swfClipEvent
// so this is what they call a color transform.
// there's a function that takes a RGB color, multiplies each component
// by mult_term and then adds the result.
typedef struct {
    RGBAColor mult_term;
    RGBAColor add_term;
} swfCXFORMWITHAPLHA;

// i should have used C++ class inheritance here, basically everything swfCMD_* inherits from this
typedef struct {
    uint8_t cmd_type;
    uint8_t flag1; // bit 0 defines if it has scale, bit 1 if it has rotation in swfCMD_placeObject2 & clipEvent
    uint8_t useless;
    uint8_t flag2; // used by swfCMD_removeObject2 (maybe its the ID of the object to be removed or depth? Who knows)
    uint32_t next_CMD; // next command in the linked list
    uint32_t vtable;
} swfCMD_header;

typedef enum {
    CMD_PLACEOBJECT2 = 0,
    CMD_CLIPEVENT = 1,
    CMD_REMOVEOBJECT = 2,
    CMD_DOACTION = 3,
} swfCMD_types;

// command for adding stuff in the screen
typedef struct {
    // insert swfCMD_Header here
    swfCMD_header header;
    uint16_t depth; 
    uint16_t character_id; // if character is 0xFFFF, that means a eaew character needs to be created
    uint32_t packed_matrix_ptr; 
    uint32_t color_xform_ptr;
} swfCMD_placeObject2;

typedef struct {
    uint32_t embedding_type;
    uint32_t avm1_code_ptr; 
} swfCMD_clipEvent_embedding;

// command for adding some code that executes when some event happens
// like moving the mouse cursor or whatever
typedef struct {
    // insert swfCMD_Header here

    swfCMD_header header;
    uint16_t depth; 
    uint16_t character_id; // if character is 0xFFFF, that means a eaew character needs to be created
    uint32_t packed_matrix_ptr; 
    uint32_t color_xform_ptr;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t code_wrapper_ptr; // can be null
    uint32_t name_ptr; // pointer to a standard ascii string that has the name of the movie clip
} swfCMD_clipEvent;

// command for you guessed it, removing stuff
typedef struct {
    swfCMD_header header;
    // insert swfCMD_Header here
    // this command does not seem to be bigger
} swfCMD_removeObject2;

// command for executing code
typedef struct {
    swfCMD_header header;
    uint32_t avm1_code_ptr; // points to the actual start of the code stream, no intermediate pointers like swfClipEvent
} swfCMD_doAction;

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

// swfSHAPE utility classes

// a swfSHAPE can sometimes have a gradient
typedef struct {
    swfMATRIX gradient_matrix; // some matrix of floats
    uint16_t stops_count; // quantity of colors in the gradients
    char pad1[0x2];
    uint32_t ratio_ptr; // pointer to an array of uint8_t
    uint32_t colors_ptr;// pointer to a rgba array
} swfGRADIENT;

// data from the fillstyle swfShape command
typedef struct {
    uint8_t unk1; // no correlation to anything, maybe flags? not sure
    char tak_marker[3]; // for some reason there's a random ascii sequence 'tak'
    RGBAColor color_data; // rgba
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
    RGBAColor color;
} swfSHAPE_StrokeStyle_data;

typedef struct {
    uint16_t opcode; // 5
    uint16_t style_index;
} swfSHAPE_StrokeStyle;

// okay this one functions in a similar way as swfTEXT,
// in that it is a state machine, for example
// set color to white
// draw these vertices filled
// set stroke width to 4
// draw these next vertices as a stroke
// blah blah blah
// its like HTML5 canvas actually
typedef struct {
    swfOBJECT_header header;
    uint32_t fill_style_table;
    uint32_t stroke_style_table;
    uint32_t display_list_ptr;
} swfSHAPE;

// text alignment
typedef enum {
    AL_LEFT = 0,
    AL_RIGHT,
    AL_CENTER,
    AL_JUSTIFY,
} swfEDITTEXT_alignment;

// this struct represents a dynamic text,
// not really sure how it works actually.
typedef struct {
    // zero idea if this is right
    uint32_t initial_text;                
    uint32_t lookup_key;

    uint16_t has_initial_text;
    int16_t leading;

    // confident on this
    RGBAColor color;

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

// this is the AVM1 parsing part
// AVM1 is the virtual machine of flash, when a project is exported
// the action script code is turned into AVM1 bytecode.

typedef struct {
    uint16_t unk1; // 01 00
    // then you have the AVM1 bytecode, i'm not gonna bother representing in the struct
    // remember that avm1 bytecode functions similar to a c string,
    // that there's a specific "instruction" that marks the end of the code.
    // shoutout to bruno for making imhex pattern code that parses the AVM1 bytecode
} AVM1Bytecode;

typedef enum {
    AC_END = 0,
    AC_PLAY = 0x06,
    AC_STOP = 0x07,
    AC_TOGGLE_QUALITY = 0x08,
    AC_STOP_SOUNDS = 0x09,
    AC_ADD = 0x0a,
    AC_SUBTRACT = 0x0b,
    AC_STRING_EQUALS = 0x13,
    AC_MULTIPLY = 0x0c,
    AC_DIVIDE = 0x0d,
    AC_OR = 0x11,
    AC_NOT = 0x12,
    AC_TO_INT = 0x18,
    AC_GETVAR = 0x1C,
    AC_SETVAR = 0x1D,
    AC_STRINGADD = 0x21,
    AC_STRING_EXTRACT = 0x15,
    AC_NEW_OBJ = 0x40,
    AC_GET_MEMBER = 0x4E,
    AC_SET_MEMBER = 0X4F,
    AC_ADD2 = 0x47,
    AC_LESS2 = 0x48,
    AC_EQUALS2 = 0x49,
    AC_INCREMENT = 0x50,
    AC_CALLMETHOD = 0x52,
    AC_GREATER = 0x67, // i'm boutta kms
    AC_CONSTANTPOOL = 0x88,
    AC_WAIT_FOR_FRAME = 0x8a,
    AC_PUSH = 0x96,
    AC_POP = 0X17,
    AC_DECREMENT = 0x51,
    AC_IF = 0x9D,
    AC_JUMP = 0x99,
    AC_GOTO_FRAME = 0x81,
    AC_PUSH_DUPLICATE = 0X4C,
    AC_TRACE = 0x26,
} AVM1Opcodes;

typedef struct {
    uint8_t opcode; // 0x88
    uint16_t length;
    uint16_t constant_count;
    char strings[];
} swfAction_ConstantPool;

typedef enum {
    SP_STRING = 0, // null terminated string
    SP_FLOAT, // 32 bit float
    // i suppose these two dont store any data,
    // flash docs doesn't make that explicit
    SP_NULL,
    SP_UNDEFINED,
    SP_REGISTER, // ui8
    SP_BOOLEAN, // ui8
    SP_DOUBLE,  // 64 bit float
    SP_INTEGER, // ui32 little endian
    SP_CONSTANT8, // constant pool index for indices < 256 (ui8)
    SP_CONSTANT16, // constant pool index for indicies >= 256 (ui16)
} swfAction_StackPush_Types;

typedef enum {
    OBJ_FILE  = 0,
    OBJ_SHAPE = 1,
    OBJ_SPRITE = 2,
    OBJ_BUTTON = 3,
    OBJ_BITMAP = 4,
    OBJ_FONT = 5,
    OBJ_TEXT = 6,
    OBJ_EDITTEXT = 7
} swfOBJECT_types;

typedef struct {
    uint8_t opcode;
    uint16_t length;
    uint8_t data[];
} swfAction_StackPush;

typedef struct {
    uint8_t opcode;
    uint16_t length; // always 3
    uint16_t frame; // frame to wait for, dunno why they call WORD 16 bits, kinda weird
    uint8_t skip_count; // number of actions to skip if frame is not loaded
} swfAction_WaitForFrame;

typedef struct {
    uint8_t opcode;
    uint16_t length;
    uint16_t offset;
} swfAction_if;

typedef struct {
    uint8_t opcode;
    uint16_t length;
    uint16_t frame_idx;
} swfAction_gotoFrame;

typedef struct {
    uint8_t opcode;
    uint16_t length;
    uint16_t offset;
} swfAction_jump;
#pragma pack(pop) // End packed struct

extern char swfObjectTypesString[10][16];
extern char swfCmdTypesString[5][32];

// this one is your bread and butter for navigating around the file
// for some examples on how to use these functions
// head over to the examples
void *pckData_get_ptr_from_og(PckData *data, uint32_t og_addr);
uint32_t pckData_get_og_from_ptr(PckData* data, void* ptr);
uint32_t pckData_get_rel_from_og(PckData* data, uint32_t og_addr);
swfOBJECT_header *pckData_get_obj(PckData *data, int pos);
int pckData_init(PckData* data, char* filename);
void pckData_free(PckData* data);

RGBAColor add_color(RGBAColor c1, RGBAColor c2);
RGBAColor mult_color(RGBAColor c1, RGBAColor c2);
RGBAColor xform_color(RGBAColor c1, swfCXFORMWITHAPLHA x);

void swfBITMAP_extract_4bpp(PckData* pck, bmpInfo1* info, RGBAColor** colors);
void swfBITMAP_extract_8bpp(PckData* pck, bmpInfo1 *info, RGBAColor **colors, uint8_t swizzle);

uint32_t avm1_size(AVM1Bytecode* code);

swfFRAME *swfSPRITE_getframe(PckData *data, uint32_t frames_og, uint32_t position);

#ifdef __cplusplus
}
#endif