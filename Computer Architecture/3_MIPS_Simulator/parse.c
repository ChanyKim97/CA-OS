/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   SCE212 Ajou University                                    */
/*   parse.c                                                   */
/*   Adapted from CS311@KAIST                                  */
/*                                                             */
/***************************************************************/

#include <stdio.h>
#include <string.h>

#include "util.h"
#include "parse.h"
#include "run.h"

int text_size;
int data_size;

instruction parsing_instr(const char *buffer, const int index)
{
    instruction instr;
	/** Implement this function */
    char* tempbuf = (char*)buffer;
    instr.value = fromBinary(tempbuf);
    //mem_write_32(MEM_TEXT_START + index, instr.value);//           const 처리필요

    //크기 + '\0'
    char _opcode[7], _func[7];
    char _rs[6], _rt[6], _rd[6], _shamt[6];
    char _imm[17] = { 0 };//                                        segmentation fault
    char _target[27];

    strncpy(_opcode, tempbuf, 6);
    strncpy(_rs, tempbuf + 6, 5);
    strncpy(_rt, tempbuf + 11, 5);
    strncpy(_rd, tempbuf + 16, 5);
    strncpy(_shamt, tempbuf +21, 5);
    strncpy(_func, tempbuf + 26, 6);
    strncpy(_imm, tempbuf + 16, 16);
    strncpy(_target, tempbuf + 6, 26);

    SET_OPCODE(&instr, fromBinary(_opcode));

    switch(OPCODE(&instr)) {
    //    //R format                      J format                        I format 
    //    //0x000000                      0x000010                        그 외 전부
    //    //                              0x000011

        //R
        case 0x0:
            SET_RS(&instr, fromBinary(_rs));
            SET_RT(&instr, fromBinary(_rt));
            SET_RD(&instr, fromBinary(_rd));
            SET_SHAMT(&instr, fromBinary(_shamt));
            SET_FUNC(&instr, fromBinary(_func));
            break;

        //J
        case 0x2:
        case 0x3:
            SET_TARGET(&instr, fromBinary(_target));
            break;

        //I
        case 0xc:
        case 0xf:
        case 0xd:
        case 0x9:
        case 0xb:
        case 0x23:
        case 0x2b:
        case 0x4:
        case 0x5:
            SET_RS(&instr, fromBinary(_rs));
            SET_RT(&instr, fromBinary(_rt));
            SET_IMM(&instr, fromBinary(_imm));
            break;
    }

    return instr;
}

void parsing_data(const char *buffer, const int index)
{
    /** Implement this function */
    char* tempbuf = (char *)buffer;
    mem_write_32(MEM_DATA_START + index, fromBinary(tempbuf)); //           const 처리필요
    //error                                                                 MEM_DATA_SIZE XXXXXXXXXXX
}

void print_parse_result()
{
    int i;
    printf("Instruction Information\n");

    for(i = 0; i < text_size/4; i++)
    {
        printf("INST_INFO[%d].value : %x\n",i, INST_INFO[i].value);
        printf("INST_INFO[%d].opcode : %d\n",i, INST_INFO[i].opcode);

	    switch(INST_INFO[i].opcode)
        {
            //Type I
            case 0x9:		//(0x001001)ADDIU
            case 0xc:		//(0x001100)ANDI
            case 0xf:		//(0x001111)LUI	
            case 0xd:		//(0x001101)ORI
            case 0xb:		//(0x001011)SLTIU
            case 0x23:		//(0x100011)LW
            case 0x2b:		//(0x101011)SW
            case 0x4:		//(0x000100)BEQ
            case 0x5:		//(0x000101)BNE
                printf("INST_INFO[%d].rs : %d\n",i, INST_INFO[i].r_t.r_i.rs);
                printf("INST_INFO[%d].rt : %d\n",i, INST_INFO[i].r_t.r_i.rt);
                printf("INST_INFO[%d].imm : %d\n",i, INST_INFO[i].r_t.r_i.r_i.imm);
                break;

            //TYPE R
            case 0x0:		//(0x000000)ADDU, AND, NOR, OR, SLTU, SLL, SRL, SUBU  if JR
                printf("INST_INFO[%d].func_code : %d\n",i, INST_INFO[i].func_code);
                printf("INST_INFO[%d].rs : %d\n",i, INST_INFO[i].r_t.r_i.rs);
                printf("INST_INFO[%d].rt : %d\n",i, INST_INFO[i].r_t.r_i.rt);
                printf("INST_INFO[%d].rd : %d\n",i, INST_INFO[i].r_t.r_i.r_i.r.rd);
                printf("INST_INFO[%d].shamt : %d\n",i, INST_INFO[i].r_t.r_i.r_i.r.shamt);
                break;

            //TYPE J
            case 0x2:		//(0x000010)J
            case 0x3:		//(0x000011)JAL
                printf("INST_INFO[%d].target : %d\n",i, INST_INFO[i].r_t.target);
                break;

            default:
                printf("Not available instruction\n");
                assert(0);
        }
    }

    printf("Memory Dump - Text Segment\n");
    for(i = 0; i < text_size; i+=4)
        printf("text_seg[%d] : %x\n", i, mem_read_32(MEM_TEXT_START + i));
    for(i = 0; i < data_size; i+=4)
        printf("data_seg[%d] : %x\n", i, mem_read_32(MEM_DATA_START + i));
    printf("Current PC: %x\n", CURRENT_STATE.PC);
}
