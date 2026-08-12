#include "../../flashpck.h"
#include <stdio.h>
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

PckData pck;

typedef struct {
    uint32_t key;
    int value;
} SeenStyle;

SeenStyle *seen_styles = NULL;

void print_color(RGBAColor c) {
    printf("\x1b[38;2;%d;%d;%dm", c.r, c.g, c.b);
    printf("#%.2X%.2X%.2X%.2X", c.r, c.g, c.b, c.a);
    printf("\x1b[0m");
}

void list_place(int frame_count, uint32_t first_frame) {
    for(int f_i = 0; f_i < frame_count; f_i++) {
        swfFRAME* frame =  swfSPRITE_getframe(&pck, first_frame, f_i);
        if(frame->commands == 0) continue;

        swfCMD_header* cmd = pckData_get_ptr_from_og(&pck, frame->commands);

        do {
            if(cmd->cmd_type == CMD_PLACEOBJECT2 || cmd->cmd_type == CMD_CLIPEVENT) {
                swfCMD_placeObject2* pobj = (swfCMD_placeObject2*)cmd;
                //printf("depth: %d\n", pobj->depth);
                if(pobj->character_id != 0xffff) {
                    swfOBJECT_header* header = pckData_get_obj(&pck, pobj->character_id);
                    switch(header->objectType) {
                        case SWF_OBJ_SPRITE: {
                            swfSPRITE* sprite = (swfSPRITE*)header;
                            break;
                        }
                        case SWF_OBJ_SHAPE: {
                            swfSHAPE* shape = (swfSHAPE*)header;
                            shape->display_list_ptr;
                            swfSHAPE_FillStyle_data* fs_data = pckData_get_ptr_from_og(
                                &pck, shape->fill_style_table
                            );
                            swfSHAPE_StrokeStyle_data* ss_data = pckData_get_ptr_from_og(
                                &pck, shape->stroke_style_table
                            );

                            uint16_t* opcode = pckData_get_ptr_from_og(&pck, shape->display_list_ptr);
                            uint8_t ended = 0;
                            int current_fill = -1;
                            ended = 0;

                            while (!ended) {
                                switch (*opcode) {
                                
                                    case SH_END:
                                        ended = 1;
                                        break;
                                
                                    case SH_DRAW_STROKES:
                                    case SH_DRAW_FILL: {
                                        swfSHAPE_Polygon *poly = (swfSHAPE_Polygon *)opcode;
                                        opcode = (uint16_t *)&poly->points[poly->vertex_count];
                                        break;
                                    }
                        
                                    case SH_STROKESTYLE_CHANGE: {
                                        swfSHAPE_StrokeStyle *ss =
                                            (swfSHAPE_StrokeStyle*)opcode;
                                    
                                        swfSHAPE_StrokeStyle_data *adata =
                                            &ss_data[ss->style_index - 1];
                                    
                                        uint32_t strokestyle_address =
                                            pckData_get_og_from_ptr(&pck, adata);
                                    
                                        if(hmgeti(seen_styles, strokestyle_address) != -1) {
                                            opcode += 2;
                                            break;
                                        }
                                    
                                        hmput(seen_styles, strokestyle_address, 1);
                                    
                                        uint32_t color_address =
                                            pckData_get_og_from_ptr(&pck, &adata->color);
                                    
                                        printf("STROKE: 0x%.08x ",
                                            color_address + 0x80 - pck.og_base_address);
                                        print_color(adata->color);
                                        printf("\n");
                                        
                                        opcode += 2;
                                        break;
                                    }
                                    case SH_FILLSTYLE_CHANGE: {
                                        swfSHAPE_FillStyle *fs = (swfSHAPE_FillStyle *)opcode;

                                        swfSHAPE_FillStyle_data* adata =
                                            &fs_data[fs->style_index - 1];

                                        uint32_t fillstyle_address =
                                            pckData_get_og_from_ptr(&pck, adata);

                                        if(hmgeti(seen_styles, fillstyle_address) != -1) {
                                            opcode += 2;
                                            break;
                                        }
                                    
                                        hmput(seen_styles, fillstyle_address, 1);
                                    
                                        uint32_t color_address =
                                            pckData_get_og_from_ptr(&pck, &adata->color_data);
                                    
                                        printf("SHAPE: 0x%.08x ",
                                               color_address + 0x80 - pck.og_base_address);
                                        print_color(adata->color_data);
                                        printf("\n");
                                        
                                        if(adata->gradient_pointer != 0) {
                                            swfGRADIENT* grad =
                                                pckData_get_ptr_from_og(&pck, adata->gradient_pointer);
                                        
                                            printf("GRADIENT BEGIN\n");
                                        
                                            RGBAColor* colors =
                                                pckData_get_ptr_from_og(&pck, grad->colors_ptr);
                                        
                                            for(size_t i = 0; i < grad->stops_count; i++) {
                                                uint32_t address =
                                                    pckData_get_og_from_ptr(&pck, &colors[i]);
                                            
                                                printf("0x%.08x ", address + 0x80 - pck.og_base_address);
                                                print_color(colors[i]);
                                                printf("\n");
                                            }
                                        
                                            printf("GRADIENT END\n");
                                        }
                                    
                                        opcode += 2;
                                        break;
                                    }                                   
                                }
                            }
                        }
                    }
                }
                if(pobj->color_xform_ptr != 0) {
                    swfCXFORMWITHAPLHA* xform= (swfCXFORMWITHAPLHA*)pckData_get_ptr_from_og(
                        &pck, 
                        pobj->color_xform_ptr
                    );
                    printf("XFORM: 0x%.08x ", pckData_get_og_from_ptr(&pck, &pobj->color_xform_ptr) + 0x80 - pck.og_base_address );
                    RGBAColor xformed = xform_color((RGBAColor){0xFF, 0xFF, 0xFF, 0xFF}, *xform);
                    print_color(xformed);
                    printf("\n");
                }
            }

            if(cmd->next_CMD == 0) break;
            cmd = pckData_get_ptr_from_og(&pck, cmd->next_CMD);
        } while(1);
    }
}

void look_for_text_colors(uint32_t text_records_ptr) {
    uint16_t* text_record_type = pckData_get_ptr_from_og(&pck, text_records_ptr);

    uint8_t ended_text = 0;
    while(!ended_text) {
        switch(*text_record_type) {
            case TR_END:
                ended_text = 1;
                break;
            case TR_COLOR: {
                TextRecord_Color* tr_color = (TextRecord_Color*)text_record_type;
                printf("TEXT: 0x%.8x ", pckData_get_og_from_ptr(&pck, &tr_color->color_rgba) + 0x80 - pck.og_base_address);
                print_color(tr_color->color_rgba);
                printf("\n");
                text_record_type = (uint16_t*)(((TextRecord_Color*)text_record_type) + 1);
                break;
            }
            case TR_FONT:{
                text_record_type = (uint16_t*)(((TextRecord_FontConfig*)text_record_type) + 1);
                break;
            }
            case TR_XOFFSET:
            case TR_YOFFSET:
                //printf("text offset\n");
                text_record_type = (uint16_t*)(((TextRecord_Offset*)text_record_type) + 1);
                break;
            case TR_GLYPHENTRY:
                TextRecord_GlyphArray* glyph_array = (TextRecord_GlyphArray*)text_record_type;
                text_record_type = (uint16_t*)&glyph_array->entries[glyph_array->glyph_count];
                break;
            default:
                ended_text = 1;
                break;
        }
    }
}

int main(int argc, char** argv) {
    pckData_init(&pck, argv[1]);

    swfFILE* f = (swfFILE*)pck.actual_data;

    list_place(f->frame_count, f->frames);

    for(int o_i = 1; o_i < f->objectCount; o_i++) {
        swfOBJECT_header* obj = pckData_get_obj(&pck, o_i);
        if(obj->objectType == SWF_OBJ_SPRITE) {
            swfSPRITE* sprite = (swfSPRITE*)obj;
            list_place(sprite->frame_count32, sprite->frames_ptr);
        } else if (obj->objectType == SWF_OBJ_TXT) {
            swfTEXT* text = (swfTEXT*)obj;
            look_for_text_colors(text->text_records_ptr);
        } else if(obj->objectType == SWF_OBJ_EDITTEXT) {
            swfEDITTEXT* edittt = (swfEDITTEXT*)obj;
            printf("EDITTEXT: 0x%.08x ", pckData_get_og_from_ptr(&pck, &edittt->color) + 0x80 - pck.og_base_address);
            print_color(edittt->color);
            printf("\n");
        }
    }
    printf("\n");
    printf("TYPE: ADDRESS COLOR");
    printf("SHAPE means it is a shape or image\n");
    printf("XFORM: color transforms that modifies SHAPE, GRADIENT and STROKE colors, "
        "it is 8 bytes, if you thing your shape is too dark or too bright, change the FIRST 4 bytes to 40 40 40 40 and the "
        "LAST 4 to 00 00 00 00.\n");
    printf("TEXT: Static text\n");
    printf("EDITTEXT: Dynamic text, that yellow text in the loading screen is also a dynamic text\n");
    printf("If you have any problems, HMU on discord, i'm rato.jpg\n");
}