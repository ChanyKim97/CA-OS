/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   SCE212 Ajou University                                    */
/*   run.c                                                     */
/*   Adapted from CS311@KAIST                                  */
/*                                                             */
/***************************************************************/

#include <stdio.h>

#include "util.h"
#include "run.h"

/***************************************************************/
/*                                                             */
/* Procedure: get_inst_info                                    */
/*                                                             */
/* Purpose: Read insturction information                       */
/*                                                             */
/***************************************************************/
instruction* get_inst_info(uint32_t pc)
{
    return &INST_INFO[(pc - MEM_TEXT_START) >> 2];
}

/***************************************************************/
/*                                                             */
/* Procedure: process_instruction                              */
/*                                                             */
/* Purpose: Process one instrction                             */
/*                                                             */
/***************************************************************/

void process_instruction()
{
	/** Implement this function */
    instruction instr;
    instr = *get_inst_info(CURRENT_STATE.PC);
	//uint32_t changePC = CURRENT_STATE.PC + 0x4;

	if (CURRENT_STATE.PC == (NUM_INST * BYTES_PER_WORD + MEM_TEXT_START)) {
		RUN_BIT = FALSE;
		return;
	}

	switch (OPCODE(&instr))
	{
		//R		ADDU AND NOR OR JR SLTU SLL SRL SUBU 
		//      21	 24	 27	 25	8  2b	0	2	23
	case 0x0:
		switch (FUNC(&instr))
		{
		case 0x21:
			CURRENT_STATE.REGS[RD(&instr)] = CURRENT_STATE.REGS[RS(&instr)] + CURRENT_STATE.REGS[RT(&instr)];
			CURRENT_STATE.PC += BYTES_PER_WORD;
			break;
		case 0x24:
			CURRENT_STATE.REGS[RD(&instr)] = CURRENT_STATE.REGS[RS(&instr)] & CURRENT_STATE.REGS[RT(&instr)];
			CURRENT_STATE.PC += BYTES_PER_WORD;
			break;
		case 0x27:
			CURRENT_STATE.REGS[RD(&instr)] = ~(CURRENT_STATE.REGS[RS(&instr)] | CURRENT_STATE.REGS[RT(&instr)]);
			CURRENT_STATE.PC += BYTES_PER_WORD;
			break;
		case 0x25:
			CURRENT_STATE.REGS[RD(&instr)] = CURRENT_STATE.REGS[RS(&instr)] | CURRENT_STATE.REGS[RT(&instr)];
			CURRENT_STATE.PC += BYTES_PER_WORD;
			break;
		case 0x8:
			CURRENT_STATE.PC = CURRENT_STATE.REGS[RS(&instr)];
			break;
		case 0x2b:
			CURRENT_STATE.REGS[RD(&instr)] = (CURRENT_STATE.REGS[RS(&instr)] < CURRENT_STATE.REGS[RT(&instr)]) ? 1 : 0;
			CURRENT_STATE.PC += BYTES_PER_WORD;
			break;
		case 0x0:
			CURRENT_STATE.REGS[RD(&instr)] = CURRENT_STATE.REGS[RT(&instr)] << SHAMT(&instr);
			CURRENT_STATE.PC += BYTES_PER_WORD;
			break;
		case 0x2:
			CURRENT_STATE.REGS[RD(&instr)] = CURRENT_STATE.REGS[RT(&instr)] >> SHAMT(&instr);
			CURRENT_STATE.PC += BYTES_PER_WORD;
			break;
		case 0x23:
			CURRENT_STATE.REGS[RD(&instr)] = CURRENT_STATE.REGS[RS(&instr)] - CURRENT_STATE.REGS[RT(&instr)];
			CURRENT_STATE.PC += BYTES_PER_WORD;
			break;
		default:
			break;
		}
		break;								//error

	//I		ADDIU	ANDI	LUI		ORI		SLTIU	LW	SW	BEQ	BNE
	//		9		c		f		d		b		23	2b	4	5
	case 0x9:
		CURRENT_STATE.REGS[RT(&instr)] = CURRENT_STATE.REGS[RS(&instr)] + IMM(&instr);
		CURRENT_STATE.PC += BYTES_PER_WORD;
		break;
	case 0xc:
		CURRENT_STATE.REGS[RT(&instr)] = CURRENT_STATE.REGS[RS(&instr)] & IMM(&instr);
		CURRENT_STATE.PC += BYTES_PER_WORD;
		break;
	case 0xf:
		CURRENT_STATE.REGS[RT(&instr)] = (IMM(&instr) << 16);
		CURRENT_STATE.PC += BYTES_PER_WORD;
		break;
	case 0xd:
		CURRENT_STATE.REGS[RT(&instr)] = CURRENT_STATE.REGS[RS(&instr)] | IMM(&instr);
		CURRENT_STATE.PC += BYTES_PER_WORD;
		break;
	case 0xb:
		CURRENT_STATE.REGS[RT(&instr)] = (CURRENT_STATE.REGS[RS(&instr)] < IMM(&instr)) ? 1 : 0;
		CURRENT_STATE.PC += BYTES_PER_WORD;
		break;
	case 0x23:
		CURRENT_STATE.REGS[RT(&instr)] = mem_read_32(CURRENT_STATE.REGS[RS(&instr)] + IMM(&instr));
		CURRENT_STATE.PC += BYTES_PER_WORD;
		break;
	case 0x2b:
		mem_write_32(CURRENT_STATE.REGS[RS(&instr)] + IMM(&instr), CURRENT_STATE.REGS[RT(&instr)]);
		CURRENT_STATE.PC += BYTES_PER_WORD;
		break;
	case 0x4:
		if (CURRENT_STATE.REGS[RS(&instr)] == CURRENT_STATE.REGS[RT(&instr)]) {
			CURRENT_STATE.PC = CURRENT_STATE.PC + 0x4 + (IMM(&instr) << 2);
		}
		else CURRENT_STATE.PC += BYTES_PER_WORD;
		break;
	case 0x5:
		if (CURRENT_STATE.REGS[RS(&instr)] != CURRENT_STATE.REGS[RT(&instr)]) {
			CURRENT_STATE.PC = CURRENT_STATE.PC + 0x4 + (IMM(&instr) << 2);
		}
		else CURRENT_STATE.PC += BYTES_PER_WORD;
		break;

		//J		J	JAL
		//		2	3
	case 0x2:
		CURRENT_STATE.PC = TARGET(&instr) << 2;
		break;
	case 0x3:
		CURRENT_STATE.REGS[31] = CURRENT_STATE.PC + 0x8;
		CURRENT_STATE.PC = TARGET(&instr) << 2;
		break;
	}
}
