#include "../../flashpck.h"
#include <stdio.h>

PckData pck;
void list_place(int frame_count, uint32_t first_frame) {
    for(int f_i = 0; f_i < frame_count; f_i++) {
        swfFRAME* frame =  swfSPRITE_getframe(&pck, first_frame, f_i);
        if(frame->commands == 0) continue;

        swfCMD_header* cmd = pckData_get_ptr_from_og(&pck, frame->commands);

        do {
            if(cmd->cmd_type == CMD_PLACEOBJECT2) {
                swfCMD_placeObject2* pobj = (swfCMD_placeObject2*)pobj;
                if(pobj->character_id != 0xffff) {
                    if(pobj->color_xform_ptr == 0)
                        printf("transform is null\n");
                }
            }
            if(cmd->next_CMD == 0) break;
            cmd = pckData_get_ptr_from_og(&pck, cmd->next_CMD);
        } while(1);

        
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
        }
    }
}