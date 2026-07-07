#include "../../flashpck.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <libgen.h>
#include <string.h>

#ifdef _WIN32
    #include <direct.h>
    #define make_directory(path) _mkdir(path)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define make_directory(path) mkdir(path, 0777)
#endif

static char outdir[1024];

// okay, if you are running this program on a file and
// see the message "unknown opcode", that means this
// avm1 instruction wasn't implemented, that's basically
// my workflow, since i dont know if MC3 implements the 
// whole set of AVM1 instruction, i had to implement them one by one
// if that happens to you, go to the flash7 spec 
// https://www.ics.agh.edu.pl/dydaktyka/mm/lato0405_inf_d/wyklady/w4/SWF7_specification.pdf
// search for this opcode, and implement it.
// you should add an entry in the AVM1opcodes enum 
// and add a new case in parse_avm1 function
void parse_avm1(PckData* pck, AVM1Bytecode* code, char* pathname) {
    // skipping the first uint16    
    int start = pckData_get_og_from_ptr(pck, (void*)code);
    int size = avm1_size(code);

    uint8_t* opcode = (uint8_t*)(code+1);
    uint8_t ended = 0;
    char** constants = NULL; 
    FILE* f = fopen(pathname, "w");
    fprintf(f, "do action code\ns: %.8x, e: %8.x\n", 
        start,
        start + size
    );
    while(!ended) {
        fprintf(f, "0x%.8x %.2x ", pckData_get_og_from_ptr(pck ,opcode), *opcode);
        switch(*opcode) {
            case AC_END: // opcode 0x0
                ended = 1;
                fprintf(f, "end\n");
                break;
            case AC_PLAY: // opcode 0x6
                opcode++;
                fprintf(f, "play\n");
                break;
            case AC_STOP: // opcode 0x7
                fprintf(f, "stop\n");
                opcode++;
                break;
            case AC_TOGGLE_QUALITY: // opcode 0x08
                opcode++;
                fprintf(f, "toggle quality\n");
                break;
            case AC_STOP_SOUNDS: // opcode 0x09
                opcode++;
                fprintf(f, "stop sounds\n");
                break;
            case AC_ADD: // opcode 0x0a
                opcode++;
                fprintf(f, "math add\n");
                break;
            case AC_PUSH_DUPLICATE:
                opcode++;
                fprintf(f, "push duplicate\n");
                break;
            case AC_SUBTRACT:
                opcode++;
                fprintf(f, "math subtract\n");
                break;
            case AC_MULTIPLY:
                opcode++;
                fprintf(f, "math multiply\n");
                break;
            case AC_DIVIDE:
                opcode++;
                fprintf(f, "math divide\n");
                break;
            case AC_STRING_EQUALS: 
                opcode++;
                fprintf(f,"string equals\n");
                break;
            case AC_OR:
                opcode++;
                fprintf(f, "math or\n");
                break;
            case AC_NOT:
                opcode++;
                fprintf(f, "math not\n");
                break;
            case AC_STRING_EXTRACT:
                opcode++;
                fprintf(f, "string extract\n");
                break;
            case AC_TO_INT:
                opcode++;
                fprintf(f, "to integer\n");
                break;
            case AC_GETVAR: // opcode 0x1c
                opcode++;
                fprintf(f, "get variable\n");
                break;
            case AC_SETVAR: // opcode 0x1d
                opcode++;
                fprintf(f, "set variable\n");
                break;
            case AC_STRINGADD: // opcode 0x21
                opcode++;
                fprintf(f, "string concatenation\n");
                break;
            case AC_NEW_OBJ: // opcode 0x40
                opcode++;
                fprintf(f, "new object\n");
                break;
            case AC_GET_MEMBER:
                opcode++;
                fprintf(f, "get member\n");
                break;
            case AC_SET_MEMBER: // opcode 0x4f
                opcode++;
                fprintf(f, "set member\n");
                break;
            case AC_ADD2:
                opcode++;
                fprintf(f, "math add2\n");
                break;
            case AC_LESS2:
                opcode++;
                fprintf(f, "math less2\n");
                break;
            case AC_EQUALS2: // opcode 0x49
                opcode++;
                fprintf(f, "math equals2\n");
                break;
            case AC_INCREMENT: // opcode 0x50
                opcode++;
                fprintf(f, "increment\n");
                break;
            case AC_CALLMETHOD:
                opcode++;
                fprintf(f, "call method\n");
                break;
            case AC_GREATER: // opcode 0x67
                opcode++;
                fprintf(f, "math greater\n");
                break;
            case AC_CONSTANTPOOL: { // opcode 0x88
                swfAction_ConstantPool *cp = (swfAction_ConstantPool *)opcode;
                fprintf(f, "constant pool\n");

                char *constant_name = cp->strings;
                for (uint16_t i = 0; i < cp->constant_count; ++i) {
                    stbds_arrput(constants, constant_name);
                    fprintf(f, "%s\n", constant_name);
                    constant_name += strlen(constant_name) + 1;
                }
            
                opcode = (uint8_t *)(constant_name);
                break;
            }
            case AC_POP:
                fprintf(f, "pop\n");
                opcode++;
                break;
            case AC_DECREMENT:
                fprintf(f, "decrement\n");
                opcode++;
                break;
            case AC_TRACE:
                fprintf(f, "debug trace\n");
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
                        fprintf(f, "push constant8 \"%s\"\n",
                               constants[*p]);
                        p += 1;
                        break;
                        
                    case SP_CONSTANT16: {
                        uint16_t idx = *(uint16_t *)p;
                        fprintf(f, "push constant16 \"%s\"\n",
                               constants[idx]);
                        p += 2;
                        break;
                    }

                    case SP_STRING:
                        fprintf(f, "push string \"%s\"\n", (char *)p);
                        p += strlen((char *)p) + 1;
                        break;

                    case SP_FLOAT: {
                        float value = *(float *)p;
                        fprintf(f, "push float %f\n", value);
                        p += 4;
                        break;
                    }

                    case SP_DOUBLE: {
                        double value = *(double *)p;
                        fprintf(f, "push double %f\n", value);
                        p += 8;
                        break;
                    }

                    case SP_INTEGER: {
                        int32_t value = *(int32_t *)p;
                        fprintf(f, "push int %d\n", value);
                        p += 4;
                        break;
                    }

                    case SP_BOOLEAN:
                        fprintf(f, "push bool %s\n", *p ? "true" : "false");
                        p += 1;
                        break;

                    case SP_REGISTER:
                        fprintf(f, "push register %u\n", *p);
                        p += 1;
                        break;

                    case SP_NULL:
                        fprintf(f, "push null\n");
                        break;

                    case SP_UNDEFINED:
                        fprintf(f, "push undefined\n");
                        break;
                
                    default:
                        fprintf(f, "unknown push type 0x%u\n", type);
                        exit(1);
                    }
                }
                opcode = end;
                break;
            }
            case AC_GOTO_FRAME:
                swfAction_gotoFrame* a_frame = (swfAction_gotoFrame*)opcode;
                fprintf(f, "goto frame %d\n", a_frame->frame_idx);
                opcode = (uint8_t*)(a_frame+1);
                break;
            case AC_IF:
                swfAction_if* a_if = (swfAction_if*)opcode;
                fprintf(f, "goto %d if 0\n", a_if->offset);
                opcode = (uint8_t*)(a_if+1);
                break;
            case AC_JUMP:
                swfAction_jump* a_jump = (swfAction_jump*)opcode;
                fprintf(f, "goto %d\n", a_jump->offset);
                opcode = (uint8_t*)(a_jump+1);
                break;
            case AC_WAIT_FOR_FRAME: // opcode 0x8a
                swfAction_WaitForFrame* wait_ff = (swfAction_WaitForFrame*)opcode;

                fprintf(f, "wait for frame %d, skip %d codes\n", wait_ff->frame, wait_ff->skip_count);
                opcode = (uint8_t*)(wait_ff+1);
                break;
            default:
                fprintf(f, "unknown opcode\n", *opcode);
                exit(1);
                break;
        }
    }
    stbds_arrfree(constants);
}

void look_for_code(PckData* pck, uint32_t frame_ptr, uint32_t frame_count, uint32_t obj_index) {
    for(int f_i = 0; f_i < frame_count; f_i++) {
        swfFRAME* frame = swfSPRITE_getframe(pck, frame_ptr, f_i);
        if(frame->commands == 0) continue;
        swfCMD_header* header = pckData_get_ptr_from_og(pck, frame->commands);
        int c = 0;
        do {
            switch(header->cmd_type) {
                case CMD_CLIPEVENT: {


                    swfCMD_clipEvent* clip_ev = (swfCMD_clipEvent*)(header);
                    if(clip_ev->code_wrapper_ptr == 0) break; // has no embedding
                    swfCMD_clipEvent_embedding* wrapper = 
                        (swfCMD_clipEvent_embedding*)pckData_get_ptr_from_og(
                            pck, clip_ev->code_wrapper_ptr);
                    if(wrapper->embedding_type != 2) break; // embedding is not code
                    AVM1Bytecode* code = (AVM1Bytecode*)pckData_get_ptr_from_og(
                        pck, wrapper->avm1_code_ptr);

                    char fpath[1024];
                    snprintf(fpath,sizeof(fpath), "%s/%d_%d_%d.txt", outdir, obj_index, f_i, c);
                    parse_avm1(pck, code, fpath);
                    c++;
                    break;
                }
                case CMD_DOACTION: {
                    swfCMD_doAction* do_a = (swfCMD_doAction*)(header);
                    AVM1Bytecode* code = (AVM1Bytecode*)pckData_get_ptr_from_og(
                        pck, do_a->avm1_code_ptr);

                    char fpath[1024];
                    snprintf(fpath, sizeof(fpath), "%s/%d_%d_%d.txt", outdir, obj_index, f_i, c);
                    parse_avm1(pck, code, fpath);
                    c++;
                    break;
                }
                default: // nothing cuz they dont have code
                    break;
            }

            if(header->next_CMD == 0)
                break;
            header = (swfCMD_header*)pckData_get_ptr_from_og(pck, header->next_CMD);
        } while(1);
    }

}


int main(int argc, char** argv) {
    PckData pck;
    pckData_init(&pck, argv[1]);

    swfFILE* file = (swfFILE*)pck.actual_data;
    char input_copy[1024];

    strncpy(input_copy, argv[1], sizeof(input_copy));
    input_copy[sizeof(input_copy) - 1] = '\0';
    
    // Remove extension
    char *dot = strrchr(input_copy, '.');
    if (dot)
        *dot = '\0';
    
    snprintf(outdir, sizeof(outdir), "%s", input_copy);
    
    make_directory(outdir);

    // reading stuff from the swfFILE first
    look_for_code(&pck, file->frames, file->frame_count, 0);
    
    for(int o_i = 1; o_i < file->objectCount; o_i++) {
        swfOBJECT_header* header =  pckData_get_obj(&pck, o_i);
        switch(header->objectType) {
            case OBJ_SPRITE:
                swfSPRITE* sprite = (swfSPRITE*)header;
                look_for_code(&pck, sprite->frames_ptr, sprite->frame_count32, o_i);
                break;
            default:
                break;
        }
    }
}