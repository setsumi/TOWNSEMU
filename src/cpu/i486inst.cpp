#include <iostream>

#include "cpputil.h"
#include "i486.h"
#include "i486inst.h"



bool i486DX::OpCodeNeedsOneMoreByte(unsigned int firstByte) const
{
	switch(firstByte)
	{
	case I486_OPCODE_NEED_SECOND_BYTE:
	case I486_OPCODE_NEED_SECOND_BYTE_AAD://_=0xD5,
	case I486_OPCODE_NEED_SECOND_BYTE_AAM://_=0xD4,
		return true;
	}
	return false;
}


i486DX::Instruction i486DX::FetchInstruction(const SegmentRegister &CS,unsigned int offset,const Memory &mem) const
{
	Instruction inst;
	inst.Clear();
	if(true==IsInRealMode())
	{
		inst.operandSize=16;
		inst.addressSize=16;
	}
	else
	{
		// Default operandSize and addressSize depends on the D flag of the segment descriptor.
		inst.operandSize=CS.operandSize;
		inst.addressSize=CS.addressSize;
	}

	// Question: Do prefixes need to be in the specific order INST_PREFIX->ADDRSIZE_OVERRIDE->OPSIZE_OVERRIDE->SEG_OVERRIDE?

	unsigned int lastByte=FetchByte(CS,offset+inst.numBytes++,mem);
	for(;;) // While looking at prefixes.
	{
		switch(lastByte)
		{
		case INST_PREFIX_REP: // REP/REPE/REPZ
		case INST_PREFIX_REPNE:
		case INST_PREFIX_LOCK:
			inst.instPrefix=lastByte;
			break;

		case SEG_OVERRIDE_CS:
		case SEG_OVERRIDE_SS:
		case SEG_OVERRIDE_DS:
		case SEG_OVERRIDE_ES:
		case SEG_OVERRIDE_FS:
		case SEG_OVERRIDE_GS:
			inst.segOverride=lastByte;
			break;

		case OPSIZE_OVERRIDE:
			inst.operandSize^=48;
			break;
		case ADDRSIZE_OVERRIDE:
			inst.addressSize^=48;
			break;
		default:
			goto PREFIX_DONE;
		}
		lastByte=FetchByte(CS,offset+inst.numBytes++,mem);
	}
PREFIX_DONE:
	inst.opCode=lastByte;
	if(true==OpCodeNeedsOneMoreByte(inst.opCode))
	{
		lastByte=FetchByte(CS,offset+inst.numBytes++,mem);
		inst.opCode|=(lastByte<<8);
	}
	if(inst.opCode==0xDB)
	{
		auto nextByte=FetchByte(CS,offset+inst.numBytes,mem);
		if(0xE3==nextByte
		   // || ??==nextByte
		)
		{
			lastByte=nextByte;
			inst.opCode|=(lastByte<<8);
			++inst.numBytes;
		}
	}

	FetchOperand(inst,CS,offset+inst.numBytes,mem);

	return inst;
}

unsigned int i486DX::FetchOperand8(Instruction &inst,const SegmentRegister &seg,unsigned int offset,const Memory &mem) const
{
	auto byte=FetchByte(seg,offset++,mem);
	inst.operand[inst.operandLen++]=byte;
	++inst.numBytes;
	return 1;
}
unsigned int i486DX::FetchOperand16(Instruction &inst,const SegmentRegister &seg,unsigned int offset,const Memory &mem) const
{
	unsigned int byte[2];
	byte[0]=FetchByte(seg,offset++,mem);
	byte[1]=FetchByte(seg,offset++,mem);

	inst.operand[inst.operandLen++]=byte[0];
	inst.operand[inst.operandLen++]=byte[1];
	inst.numBytes+=2;
	return 2;
}
unsigned int i486DX::FetchOperand32(Instruction &inst,const SegmentRegister &seg,unsigned int offset,const Memory &mem) const
{
	unsigned int byte[4];
	byte[0]=FetchByte(seg,offset++,mem);
	byte[1]=FetchByte(seg,offset++,mem);
	byte[2]=FetchByte(seg,offset++,mem);
	byte[3]=FetchByte(seg,offset++,mem);
	inst.operand[inst.operandLen++]=byte[0];
	inst.operand[inst.operandLen++]=byte[1];
	inst.operand[inst.operandLen++]=byte[2];
	inst.operand[inst.operandLen++]=byte[3];
	inst.numBytes+=4;
	return 4;
}

unsigned int i486DX::FetchOperand16or32(Instruction &inst,const SegmentRegister &seg,unsigned int offset,const Memory &mem) const
{
	if(16==inst.operandSize)
	{
		return FetchOperand16(inst,seg,offset,mem);
	}
	else // if(32==inst.operandSize)
	{
		return FetchOperand32(inst,seg,offset,mem);
	}
}

unsigned int i486DX::FetchOperandRM(Instruction &inst,const SegmentRegister &seg,unsigned int offset,const Memory &mem) const
{
	offset+=FetchOperand8(inst,seg,offset,mem);

	unsigned int numBytesFetched=1;
	auto MODR_M=inst.operand[inst.operandLen-1];
	auto MOD=(MODR_M>>6)&3;
	auto R_M=(MODR_M)&7;

	// [1] Table 26-1, 26-2, 26-3, pp. 26-4,26-5,26-6
	if(16==inst.addressSize)
	{
		if(0b00==MOD && 0b110==R_M) // disp16
		{
			numBytesFetched+=FetchOperand16(inst,seg,offset,mem);
		}
		else if(0b01==MOD)
		{
			numBytesFetched+=FetchOperand8(inst,seg,offset,mem);
		}
		else if(0b10==MOD)
		{
			numBytesFetched+=FetchOperand16(inst,seg,offset,mem);
		}
	}
	else // if(32==inst.addressSize)
	{
		if(0b00==MOD)
		{
			if(0b100==R_M) // SIB
			{
				FetchOperand8(inst,seg,offset,mem);
				++numBytesFetched;
				++offset;

				auto SIB=inst.operand[inst.operandLen-1];
				auto BASE=(SIB&7);
				// Special case MOD=0b00 && BASE==5 [1] Table 26-4 pp.26-7
				// No base, [disp32+scaled_index]

				numBytesFetched+=FetchOperand32(inst,seg,offset,mem);
			}
			else if(0b101==R_M) // disp32
			{
				numBytesFetched+=FetchOperand32(inst,seg,offset,mem);
			}
		}
		else if(0b01==MOD)
		{
			if(0b100==R_M) // SIB+disp8
			{
				FetchOperand8(inst,seg,offset,mem);
				++numBytesFetched;
				++offset;
			}
			FetchOperand8(inst,seg,offset,mem);
			++numBytesFetched;
		}
		else if(0b10==MOD)
		{
			if(0b100==R_M) // SIB+disp32
			{
				FetchOperand8(inst,seg,offset,mem);
				++numBytesFetched;
				++offset;
			}
			numBytesFetched+=FetchOperand32(inst,seg,offset,mem);
		}
	}

	return numBytesFetched;
}

void i486DX::FetchOperand(Instruction &inst,const SegmentRegister &seg,int offset,const Memory &mem) const
{
	switch(inst.opCode)
	{
	case I486_OPCODE_C0_ROL_ROR_RCL_RCR_SAL_SAR_SHL_SHR_RM8_I8://=0xC0,// ::ROL(REG=0),ROR(REG=1),RCL(REG=2),RCR(REG=3),SAL/SHL(REG=4),SHR(REG=5),SAR(REG=7)
	case I486_OPCODE_C1_ROL_ROR_RCL_RCR_SAL_SAR_SHL_SHR_RM_I8:// =0xC1, // ROL(REG=0),ROR(REG=1),RCL(REG=2),RCR(REG=3),SAL/SHL(REG=4),SHR(REG=5),SAR(REG=7)
		offset+=FetchOperandRM(inst,seg,offset,mem);
		FetchOperand8(inst,seg,offset,mem);
		break;
	case I486_OPCODE_D1_ROL_ROR_RCL_RCR_SAL_SAR_SHL_SHR_RM8_1://=0xD1, // ROL(REG=0),ROR(REG=1),RCL(REG=2),RCR(REG=3),SAL/SHL(REG=4),SHR(REG=5),SAR(REG=7)
		offset+=FetchOperandRM(inst,seg,offset,mem);
		break;


	case I486_OPCODE_F6_TEST_NOT_NEG_MUL_IMUL_DIV_IDIV: //=0xF6
	case I486_OPCODE_F7_TEST_NOT_NEG_MUL_IMUL_DIV_IDIV: //=0xF7,
		break;


	case I486_OPCODE_CALL_REL://   0xE8,
	case I486_OPCODE_JMP_REL://          0xE9,   // cw or cd
		FetchOperand16or32(inst,seg,offset,mem);
		break;
	case I486_OPCODE_CALL_FAR://   0x9A,
	case I486_OPCODE_JMP_FAR:
		offset+=FetchOperand16or32(inst,seg,offset,mem);
		FetchOperand16(inst,seg,offset,mem);
		break;


	case I486_OPCODE_CLD:
	case I486_OPCODE_CLI:
		break;


	case I486_OPCODE_DEC_EAX:
	case I486_OPCODE_DEC_ECX:
	case I486_OPCODE_DEC_EDX:
	case I486_OPCODE_DEC_EBX:
	case I486_OPCODE_DEC_ESP:
	case I486_OPCODE_DEC_EBP:
	case I486_OPCODE_DEC_ESI:
	case I486_OPCODE_DEC_EDI:
		break;


	case I486_OPCODE_FNINIT:
		break;


	case I486_OPCODE_IN_AL_I8://=        0xE4,
	case I486_OPCODE_IN_A_I8://=         0xE5,
		FetchOperand8(inst,seg,offset,mem);
		break;
	case I486_OPCODE_IN_AL_DX://=        0xEC,
	case I486_OPCODE_IN_A_DX://=         0xED,
		break;


	case I486_OPCODE_INC_DEC_R_M8:
		FetchOperandRM(inst,seg,offset,mem);
		break;
	case I486_OPCODE_INC_DEC_CALL_CALLF_JMP_JMPF_PUSH:
		FetchOperandRM(inst,seg,offset,mem);
		break;
	case I486_OPCODE_INC_EAX://    0x40, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_ECX://    0x41, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_EDX://    0x42, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_EBX://    0x43, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_ESP://    0x44, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_EBP://    0x45, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_ESI://    0x46, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_EDI://    0x47, // 16/32 depends on OPSIZE_OVERRIDE
		break;


	case I486_OPCODE_JMP_REL8://         0xEB,   // cb
	case I486_OPCODE_JO_REL8:   // 0x70,
	case I486_OPCODE_JNO_REL8:  // 0x71,
	case I486_OPCODE_JB_REL8:   // 0x72,
	case I486_OPCODE_JAE_REL8:  // 0x73,
	case I486_OPCODE_JE_REL8:   // 0x74,
	case I486_OPCODE_JNE_REL8:  // 0x75,
	case I486_OPCODE_JBE_REL8:  // 0x76,
	case I486_OPCODE_JA_REL8:   // 0x77,
	case I486_OPCODE_JS_REL8:   // 0x78,
	case I486_OPCODE_JNS_REL8:  // 0x79,
	case I486_OPCODE_JP_REL8:   // 0x7A,
	case I486_OPCODE_JNP_REL8:  // 0x7B,
	case I486_OPCODE_JL_REL8:   // 0x7C,
	case I486_OPCODE_JGE_REL8:  // 0x7D,
	case I486_OPCODE_JLE_REL8:  // 0x7E,
	case I486_OPCODE_JG_REL8:   // 0x7F,
		FetchOperand8(inst,seg,offset,mem);
		break;


	case I486_OPCODE_BINARYOP_RM8_FROM_I8:
		offset+=FetchOperandRM(inst,seg,offset,mem);
		FetchOperand8(inst,seg,offset,mem);
		break;
	case I486_OPCODE_BINARYOP_R_FROM_I:
		offset+=FetchOperandRM(inst,seg,offset,mem);
		FetchOperand16or32(inst,seg,offset,mem);
		break;
	case I486_OPCODE_BINARYOP_RM_FROM_SXI8:
		offset+=FetchOperandRM(inst,seg,offset,mem);
		FetchOperand8(inst,seg,offset,mem);
		break;


	case I486_OPCODE_LGDT_LIDT:
		FetchOperandRM(inst,seg,offset,mem);
		break;


	case I486_OPCODE_MOV_FROM_R8: //      0x88,
		// Example:  88 4c ff        MOV CL,[SI-1]     In Real Mode
		// Example:  88 10           MOV DL,[BX+SI]    In Real Mode
		// Example:  88 36 21 00     MOV DH,[021H]     In Real Mode
		// Example:  67 88 26 61 10  MOV [1061],AH     In Protected Mode -> disp16 may become disp32, and vise-versa

		// Example:  8D 04 C1        LEA EAX,[ECX+EAX*8] In Protected Mode
		// Example:  8D 04 41        LEA EAX,[ECX+EAX*2] In Protected Mode
	case I486_OPCODE_MOV_FROM_R: //       0x89, // 16/32 depends on OPSIZE_OVERRIDE
		// Example:  89 26 3e 00     MOV [003EH],SP
	case I486_OPCODE_MOV_TO_R8: //        0x8A,
		// Example:  8a 0e 16 00     MOV CL,[0016H]
	case I486_OPCODE_MOV_TO_R: //         0x8B, // 16/32 depends on OPSIZE_OVERRIDE
		FetchOperandRM(inst,seg,offset,mem);
		break;


	case I486_OPCODE_MOV_FROM_SEG: //     0x8C,
		// Example:  8c c6           MOV SI,ES
		// Sreg: ES=0, CS=1, SS=2, DS=3, FD=4, GS=5 (OPCODE part of MODR_M)  [1] pp.26-10
	case I486_OPCODE_MOV_TO_SEG: //       0x8E,
		FetchOperand8(inst,seg,offset,mem);
		break;

	case I486_OPCODE_MOV_M_TO_AL: //      0xA0,
	case I486_OPCODE_MOV_M_TO_EAX: //     0xA1, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_M_FROM_AL: //    0xA2,
	case I486_OPCODE_MOV_M_FROM_EAX: //   0xA3, // 16/32 depends on OPSIZE_OVERRIDE
		FetchOperandRM(inst,seg,offset,mem);
		break;

	case I486_OPCODE_MOV_I8_TO_AL: //     0xB0,
	case I486_OPCODE_MOV_I8_TO_CL: //     0xB1,
	case I486_OPCODE_MOV_I8_TO_DL: //     0xB2,
	case I486_OPCODE_MOV_I8_TO_BL: //     0xB3,
	case I486_OPCODE_MOV_I8_TO_AH: //     0xB4,
	case I486_OPCODE_MOV_I8_TO_CH: //     0xB5,
	case I486_OPCODE_MOV_I8_TO_DH: //     0xB6,
	case I486_OPCODE_MOV_I8_TO_BH: //     0xB7,
		FetchOperand8(inst,seg,offset,mem);
		break;
	case I486_OPCODE_MOV_I_TO_EAX: //   0xB8, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_ECX: //   0xB9, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_EDX: //   0xBA, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_EBX: //   0xBB, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_ESP: //   0xBC, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_EBP: //   0xBD, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_ESI: //   0xBE, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_EDI: //   0xBF, // 16/32 depends on OPSIZE_OVERRIDE
		FetchOperand16or32(inst,seg,offset,mem);
		break;
	case I486_OPCODE_MOV_I8_TO_RM8: //    0xC6,
		offset+=FetchOperandRM(inst,seg,offset,mem);
		FetchOperand8(inst,seg,offset,mem);
		break;
	case I486_OPCODE_MOV_I_TO_RM: //      0xC7, // 16/32 depends on OPSIZE_OVERRIDE
		offset+=FetchOperandRM(inst,seg,offset,mem);
		FetchOperand16or32(inst,seg,offset,mem);
		break;


	case I486_OPCODE_MOV_TO_CR://        0x220F,
	case I486_OPCODE_MOV_FROM_CR://      0x200F,
	case I486_OPCODE_MOV_FROM_DR://      0x210F,
	case I486_OPCODE_MOV_TO_DR://        0x230F,
	case I486_OPCODE_MOV_FROM_TR://      0x240F,
	case I486_OPCODE_MOV_TO_TR://        0x260F,
		inst.operandSize=32; // [1] pp.26-213 32bit operands are always used with these instructions, 
		                     //      regardless of the opreand-size attribute.
		FetchOperandRM(inst,seg,offset,mem);
		break;


	case I486_OPCODE_MOVZX_R_RM8://=      0xB60F,
	case I486_OPCODE_MOVZX_R32_RM16://=   0xB70F,
		FetchOperandRM(inst,seg,offset,mem);
		break;


	case I486_OPCODE_OUT_I8_AL: //        0xE6,
	case I486_OPCODE_OUT_I8_A: //         0xE7,
		FetchOperand8(inst,seg,offset,mem);
		break;
	case I486_OPCODE_OUT_DX_AL: //        0xEE,
	case I486_OPCODE_OUT_DX_A: //         0xEF,
		break;


	case I486_OPCODE_PUSHA://            0x60,
	case I486_OPCODE_PUSHF://            0x9C,
		break;


	case I486_OPCODE_PUSH_EAX://         0x50,
	case I486_OPCODE_PUSH_ECX://         0x51,
	case I486_OPCODE_PUSH_EDX://         0x52,
	case I486_OPCODE_PUSH_EBX://         0x53,
	case I486_OPCODE_PUSH_ESP://         0x54,
	case I486_OPCODE_PUSH_EBP://         0x55,
	case I486_OPCODE_PUSH_ESI://         0x56,
	case I486_OPCODE_PUSH_EDI://         0x57,
		break;
	case I486_OPCODE_PUSH_I8://          0x6A,
		FetchOperand8(inst,seg,offset,mem);
		break;
	case I486_OPCODE_PUSH_I://           0x68,
		FetchOperand16or32(inst,seg,offset,mem);
		break;
	case I486_OPCODE_PUSH_CS://          0x0E,
	case I486_OPCODE_PUSH_SS://          0x16,
	case I486_OPCODE_PUSH_DS://          0x1E,
	case I486_OPCODE_PUSH_ES://          0x06,
	case I486_OPCODE_PUSH_FS://          0xA00F,
	case I486_OPCODE_PUSH_GS://          0xA80F,
		break;


	case I486_OPCODE_POP_M://            0x8F,
		FetchOperandRM(inst,seg,offset,mem);
		break;
	case I486_OPCODE_POP_EAX://          0x58,
	case I486_OPCODE_POP_ECX://          0x59,
	case I486_OPCODE_POP_EDX://          0x5A,
	case I486_OPCODE_POP_EBX://          0x5B,
	case I486_OPCODE_POP_ESP://          0x5C,
	case I486_OPCODE_POP_EBP://          0x5D,
	case I486_OPCODE_POP_ESI://          0x5E,
	case I486_OPCODE_POP_EDI://          0x5F,
	case I486_OPCODE_POP_SS://           0x17,
	case I486_OPCODE_POP_DS://           0x1F,
	case I486_OPCODE_POP_ES://           0x07,
	case I486_OPCODE_POP_FS://           0xA10F,
	case I486_OPCODE_POP_GS://           0xA90F,

	case I486_OPCODE_POPA://             0x61,
	case I486_OPCODE_POPF://             0x9D,
		break;


	case I486_OPCODE_RET://              0xC3,
	case I486_OPCODE_RETF://             0xCB,
		break;
	case I486_OPCODE_RET_I16://          0xC2,
	case I486_OPCODE_RETF_I16://         0xCA,
		FetchOperand16(inst,seg,offset,mem);
		break;


	case  I486_OPCODE_ADD_AL_FROM_I8://  0x04,
	case  I486_OPCODE_AND_AL_FROM_I8://  0x24,
	case  I486_OPCODE_CMP_AL_FROM_I8://  0x3C,
	case   I486_OPCODE_OR_AL_FROM_I8://  0x0C,
	case I486_OPCODE_TEST_AL_FROM_I8://  0xA8,
	case  I486_OPCODE_XOR_AL_FROM_I8:
		FetchOperand8(inst,seg,offset,mem);
		break;
	case  I486_OPCODE_ADD_A_FROM_I://    0x05,
	case  I486_OPCODE_AND_A_FROM_I://    0x25,
	case  I486_OPCODE_CMP_A_FROM_I://    0x3D,
	case   I486_OPCODE_OR_A_FROM_I://    0x0D,
	case I486_OPCODE_TEST_A_FROM_I://    0xA9,
	case  I486_OPCODE_XOR_A_FROM_I:
		FetchOperand16or32(inst,seg,offset,mem);
		break;
	case  I486_OPCODE_ADD_RM8_FROM_R8:// 0x00,
	case  I486_OPCODE_AND_RM8_FROM_R8:// 0x20,
	case  I486_OPCODE_CMP_RM8_FROM_R8:// 0x38,
	case   I486_OPCODE_OR_RM8_FROM_R8:// 0x08,
	case I486_OPCODE_TEST_RM8_FROM_R8:// 0x84,
	case  I486_OPCODE_XOR_RM8_FROM_R8:

	case  I486_OPCODE_ADD_RM_FROM_R://   0x01,
	case  I486_OPCODE_AND_RM_FROM_R://   0x21,
	case  I486_OPCODE_CMP_RM_FROM_R://   0x39,
	case   I486_OPCODE_OR_RM_FROM_R://   0x09,
	case I486_OPCODE_TEST_RM_FROM_R://   0x85,
	case  I486_OPCODE_XOR_RM_FROM_R:

	case I486_OPCODE_ADD_R8_FROM_RM8:// 0x02,
	case I486_OPCODE_AND_R8_FROM_RM8:// 0x22,
	case I486_OPCODE_CMP_R8_FROM_RM8:// 0x3A,
	case  I486_OPCODE_OR_R8_FROM_RM8:// 0x0A,
	case I486_OPCODE_XOR_R8_FROM_RM8:

	case I486_OPCODE_ADD_R_FROM_RM://   0x03,
	case I486_OPCODE_AND_R_FROM_RM://   0x23,
	case I486_OPCODE_CMP_R_FROM_RM://   0x3B,
	case  I486_OPCODE_OR_R_FROM_RM://   0x0B,
	case I486_OPCODE_XOR_R_FROM_RM:
		FetchOperandRM(inst,seg,offset,mem);
		break;




	default:
		// Undefined operand, or probably not implemented yet.
		break;
	}
}

void i486DX::Instruction::DecodeOperand(int addressSize,int operandSize,Operand &op1,Operand &op2) const
{
	op1.Clear();
	op2.Clear();

	switch(opCode)
	{
	case I486_OPCODE_C0_ROL_ROR_RCL_RCR_SAL_SAR_SHL_SHR_RM8_I8://=0xC0,// ::ROL(REG=0),ROR(REG=1),RCL(REG=2),RCR(REG=3),SAL/SHL(REG=4),SHR(REG=5),SAR(REG=7)
		op1.Decode(addressSize,8,operand);
		op2.MakeImm8(*this);
		break;
	case I486_OPCODE_C1_ROL_ROR_RCL_RCR_SAL_SAR_SHL_SHR_RM_I8:// =0xC1, // ROL(REG=0),ROR(REG=1),RCL(REG=2),RCR(REG=3),SAL/SHL(REG=4),SHR(REG=5),SAR(REG=7)
		op1.Decode(addressSize,operandSize,operand);
		op2.MakeImm8(*this);
		break;
	case I486_OPCODE_D1_ROL_ROR_RCL_RCR_SAL_SAR_SHL_SHR_RM8_1://=0xD1, // ROL(REG=0),ROR(REG=1),RCL(REG=2),RCR(REG=3),SAL/SHL(REG=4),SHR(REG=5),SAR(REG=7)
		op1.Decode(addressSize,operandSize,operand);
		break;


	case I486_OPCODE_F6_TEST_NOT_NEG_MUL_IMUL_DIV_IDIV: //=0xF6
	case I486_OPCODE_F7_TEST_NOT_NEG_MUL_IMUL_DIV_IDIV: //=0xF7,
		break;


	case I486_OPCODE_CALL_REL://   0xE8,
	case I486_OPCODE_JMP_REL://          0xE9,   // cw or cd
		op1.MakeImm16or32(*this,operandSize);
		break;
	case I486_OPCODE_CALL_FAR://   0x9A,
	case I486_OPCODE_JMP_FAR:
		op1.DecodeFarAddr(addressSize,operandSize,operand);
		break;


	case I486_OPCODE_CLD:
	case I486_OPCODE_CLI:
		break;


	case I486_OPCODE_FNINIT:
		break;


	case I486_OPCODE_DEC_EAX:
	case I486_OPCODE_DEC_ECX:
	case I486_OPCODE_DEC_EDX:
	case I486_OPCODE_DEC_EBX:
	case I486_OPCODE_DEC_ESP:
	case I486_OPCODE_DEC_EBP:
	case I486_OPCODE_DEC_ESI:
	case I486_OPCODE_DEC_EDI:
		op1.MakeByRegisterNumber(operandSize,opCode&7);
		break;


	case I486_OPCODE_IN_AL_I8://=        0xE4,
	case I486_OPCODE_IN_A_I8://=         0xE5,
		op1.MakeImm8(*this);
		break;
	case I486_OPCODE_IN_AL_DX://=        0xEC,
	case I486_OPCODE_IN_A_DX://=         0xED,
		break;


	case I486_OPCODE_INC_DEC_R_M8:
		op1.Decode(addressSize,8,operand);
		break;
	case I486_OPCODE_INC_DEC_CALL_CALLF_JMP_JMPF_PUSH:
		op1.Decode(addressSize,operandSize,operand);
		break;
	case I486_OPCODE_INC_EAX://    0x40, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_ECX://    0x41, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_EDX://    0x42, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_EBX://    0x43, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_ESP://    0x44, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_EBP://    0x45, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_ESI://    0x46, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_EDI://    0x47, // 16/32 depends on OPSIZE_OVERRIDE
		op1.MakeByRegisterNumber(operandSize,opCode&7);
		break;


	case I486_OPCODE_JMP_REL8://         0xEB,   // cb
	case I486_OPCODE_JO_REL8:   // 0x70,
	case I486_OPCODE_JNO_REL8:  // 0x71,
	case I486_OPCODE_JB_REL8:   // 0x72,
	case I486_OPCODE_JAE_REL8:  // 0x73,
	case I486_OPCODE_JE_REL8:   // 0x74,
	case I486_OPCODE_JNE_REL8:  // 0x75,
	case I486_OPCODE_JBE_REL8:  // 0x76,
	case I486_OPCODE_JA_REL8:   // 0x77,
	case I486_OPCODE_JS_REL8:   // 0x78,
	case I486_OPCODE_JNS_REL8:  // 0x79,
	case I486_OPCODE_JP_REL8:   // 0x7A,
	case I486_OPCODE_JNP_REL8:  // 0x7B,
	case I486_OPCODE_JL_REL8:   // 0x7C,
	case I486_OPCODE_JGE_REL8:  // 0x7D,
	case I486_OPCODE_JLE_REL8:  // 0x7E,
	case I486_OPCODE_JG_REL8:   // 0x7F,
		break;


	case I486_OPCODE_LGDT_LIDT:
		op1.Decode(addressSize,operandSize,operand);
		break;


	case I486_OPCODE_BINARYOP_RM8_FROM_I8: //  0x80, // AND(REG=4), OR(REG=1), or XOR(REG=6) depends on the REG field of MODR/M
		op1.Decode(addressSize,8,operand);
		op2.MakeImm8(*this);
		break;
	case I486_OPCODE_BINARYOP_R_FROM_I://     0x81,
		op1.Decode(addressSize,operandSize,operand);
		op2.MakeImm16or32(*this,operandSize);
		break;
	case I486_OPCODE_BINARYOP_RM_FROM_SXI8:// 0x83,
		op1.Decode(addressSize,operandSize,operand);
		op2.MakeImm8(*this);
		if(16==operandSize)
		{
			op2.SignExtendImm(OPER_IMM16);
		}
		else
		{
			op2.SignExtendImm(OPER_IMM32);
		}
		break;


	case I486_OPCODE_MOV_FROM_R8: //      0x88,
		op1.Decode(addressSize,8,operand);
		op2.DecodeMODR_MForRegister(8,operand[0]);
		break;
	case I486_OPCODE_MOV_FROM_R: //       0x89, // 16/32 depends on OPSIZE_OVERRIDE
		op1.Decode(addressSize,operandSize,operand);
		op2.DecodeMODR_MForRegister(operandSize,operand[0]);
		break;
	case I486_OPCODE_MOV_TO_R8: //        0x8A,
		op2.Decode(addressSize,8,operand);
		op1.DecodeMODR_MForRegister(8,operand[0]);
		break;
	case I486_OPCODE_MOV_TO_R: //         0x8B, // 16/32 depends on OPSIZE_OVERRIDE
		op2.Decode(addressSize,operandSize,operand);
		op1.DecodeMODR_MForRegister(operandSize,operand[0]);
		break;
	case I486_OPCODE_MOV_FROM_SEG: //     0x8C,
		op1.Decode(addressSize,operandSize,operand);
		op2.DecodeMODR_MForSegmentRegister(operand[0]);
		break;
	case I486_OPCODE_MOV_TO_SEG: //       0x8E,
		op2.Decode(addressSize,operandSize,operand);
		op1.DecodeMODR_MForSegmentRegister(operand[0]);
		break;
	case I486_OPCODE_MOV_M_TO_AL: //      0xA0,
		op2.Decode(addressSize,operandSize,operand);
		op1.MakeByRegisterNumber(8,REG_AL-REG_8BIT_REG_BASE);
		break;
	case I486_OPCODE_MOV_M_TO_EAX: //     0xA1, // 16/32 depends on OPSIZE_OVERRIDE
		op2.Decode(addressSize,operandSize,operand);
		op1.MakeByRegisterNumber(operandSize,REG_AL-REG_8BIT_REG_BASE);
		break;
	case I486_OPCODE_MOV_M_FROM_AL: //    0xA2,
		op1.Decode(addressSize,operandSize,operand);
		op2.MakeByRegisterNumber(8,REG_AL-REG_8BIT_REG_BASE);
		break;
	case I486_OPCODE_MOV_M_FROM_EAX: //   0xA3, // 16/32 depends on OPSIZE_OVERRIDE
		op1.Decode(addressSize,operandSize,operand);
		op2.MakeByRegisterNumber(operandSize,REG_AL-REG_8BIT_REG_BASE);
		break;
	case I486_OPCODE_MOV_I8_TO_AL: //     0xB0,
	case I486_OPCODE_MOV_I8_TO_CL: //     0xB1,
	case I486_OPCODE_MOV_I8_TO_DL: //     0xB2,
	case I486_OPCODE_MOV_I8_TO_BL: //     0xB3,
	case I486_OPCODE_MOV_I8_TO_AH: //     0xB4,
	case I486_OPCODE_MOV_I8_TO_CH: //     0xB5,
	case I486_OPCODE_MOV_I8_TO_DH: //     0xB6,
	case I486_OPCODE_MOV_I8_TO_BH: //     0xB7,
		op1.MakeByRegisterNumber(8,opCode&7);
		op2.MakeImm8(*this);
		break;
	case I486_OPCODE_MOV_I_TO_EAX: //     0xB8, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_ECX: //     0xB9, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_EDX: //     0xBA, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_EBX: //     0xBB, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_ESP: //     0xBC, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_EBP: //     0xBD, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_ESI: //     0xBE, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_EDI: //     0xBF, // 16/32 depends on OPSIZE_OVERRIDE
		op1.MakeByRegisterNumber(operandSize,opCode&7);
		if(16==operandSize)
		{
			op2.MakeImm16(*this);
		}
		else
		{
			op2.MakeImm32(*this);
		}
		break;
	case I486_OPCODE_MOV_I8_TO_RM8: //    0xC6,
		op1.Decode(addressSize,8,operand);
		op2.MakeImm8(*this);
		break;
	case I486_OPCODE_MOV_I_TO_RM: //      0xC7, // 16/32 depends on OPSIZE_OVERRIDE
		op1.Decode(addressSize,operandSize,operand);
		op2.MakeImm16or32(*this,operandSize);
		break;


	case I486_OPCODE_MOV_TO_CR://        0x220F,  Op1=CR, OP2=R32
		op1.DecodeMODR_MForCRRegister(operand[0]);
		op2.Decode(addressSize,32,operand);
		break;
	case I486_OPCODE_MOV_FROM_CR://      0x200F,  Op1=R32, Op2=CR
		op1.Decode(addressSize,32,operand);
		op2.DecodeMODR_MForCRRegister(operand[0]);
		break;
	case I486_OPCODE_MOV_FROM_DR://      0x210F,  Op1=R32, Op2=DR
		op1.Decode(addressSize,32,operand);
		op2.DecodeMODR_MForDRRegister(operand[0]);
		break;
	case I486_OPCODE_MOV_TO_DR://        0x230F,  Op1=DR, Op2=R32
		op1.DecodeMODR_MForDRRegister(operand[0]);
		op2.Decode(addressSize,32,operand);
		break;
	case I486_OPCODE_MOV_FROM_TR://      0x240F,  Op1=R32, Op2=TR
		op1.Decode(addressSize,32,operand);
		op2.DecodeMODR_MForTRRegister(operand[0]);
		break;
	case I486_OPCODE_MOV_TO_TR://        0x260F,  Op1=TR, Op2=R32
		op1.DecodeMODR_MForTRRegister(operand[0]);
		op2.Decode(addressSize,32,operand);
		break;


	case I486_OPCODE_MOVZX_R_RM8://=      0xB60F,
		op1.DecodeMODR_MForRegister(operandSize,operand[0]);
		op2.Decode(addressSize,8,operand);
		break;
	case I486_OPCODE_MOVZX_R32_RM16://=   0xB70F,
		op1.DecodeMODR_MForRegister(32,operand[0]);
		op2.Decode(addressSize,16,operand);
		break;


	case I486_OPCODE_OUT_I8_AL: //        0xE6,
	case I486_OPCODE_OUT_I8_A: //         0xE7,
		op1.MakeImm8(*this);
		break;
	case I486_OPCODE_OUT_DX_AL: //        0xEE,
	case I486_OPCODE_OUT_DX_A: //         0xEF,
		break;


	case I486_OPCODE_PUSHA://            0x60,
	case I486_OPCODE_PUSHF://            0x9C,
		break;


	case I486_OPCODE_PUSH_EAX://         0x50,
	case I486_OPCODE_PUSH_ECX://         0x51,
	case I486_OPCODE_PUSH_EDX://         0x52,
	case I486_OPCODE_PUSH_EBX://         0x53,
	case I486_OPCODE_PUSH_ESP://         0x54,
	case I486_OPCODE_PUSH_EBP://         0x55,
	case I486_OPCODE_PUSH_ESI://         0x56,
	case I486_OPCODE_PUSH_EDI://         0x57,
		op1.MakeImm8(*this);
		break;
	case I486_OPCODE_PUSH_I8://          0x6A,
		op1.MakeImm16or32(*this,operandSize);
		break;
	case I486_OPCODE_PUSH_I://           0x68,
		break;
	case I486_OPCODE_PUSH_CS://          0x0E,
	case I486_OPCODE_PUSH_SS://          0x16,
	case I486_OPCODE_PUSH_DS://          0x1E,
	case I486_OPCODE_PUSH_ES://          0x06,
	case I486_OPCODE_PUSH_FS://          0xA00F,
	case I486_OPCODE_PUSH_GS://          0xA80F,
		break;


	case I486_OPCODE_POP_M://            0x8F,
		op1.Decode(addressSize,operandSize,operand);
		break;
	case I486_OPCODE_POP_EAX://          0x58,
	case I486_OPCODE_POP_ECX://          0x59,
	case I486_OPCODE_POP_EDX://          0x5A,
	case I486_OPCODE_POP_EBX://          0x5B,
	case I486_OPCODE_POP_ESP://          0x5C,
	case I486_OPCODE_POP_EBP://          0x5D,
	case I486_OPCODE_POP_ESI://          0x5E,
	case I486_OPCODE_POP_EDI://          0x5F,
	case I486_OPCODE_POP_SS://           0x17,
	case I486_OPCODE_POP_DS://           0x1F,
	case I486_OPCODE_POP_ES://           0x07,
	case I486_OPCODE_POP_FS://           0xA10F,
	case I486_OPCODE_POP_GS://           0xA90F,

	case I486_OPCODE_POPA://             0x61,
	case I486_OPCODE_POPF://             0x9D,
		break;


	case  I486_OPCODE_ADD_AL_FROM_I8://  0x04,
	case  I486_OPCODE_AND_AL_FROM_I8://  0x24,
	case  I486_OPCODE_CMP_AL_FROM_I8://  0x3C,
	case   I486_OPCODE_OR_AL_FROM_I8://  0x0C,
	case  I486_OPCODE_XOR_AL_FROM_I8:
	case I486_OPCODE_TEST_AL_FROM_I8://  0xA8,
		op1.MakeImm8(*this);
		break;
	case  I486_OPCODE_ADD_A_FROM_I://    0x05,
	case  I486_OPCODE_AND_A_FROM_I://    0x25,
	case  I486_OPCODE_CMP_A_FROM_I://    0x3D,
	case   I486_OPCODE_OR_A_FROM_I://    0x0D,
	case I486_OPCODE_TEST_A_FROM_I://    0xA9,
	case  I486_OPCODE_XOR_A_FROM_I:
		op1.MakeImm16or32(*this,operandSize);
		break;
	case  I486_OPCODE_ADD_RM8_FROM_R8:// 0x00,
	case  I486_OPCODE_AND_RM8_FROM_R8:// 0x20,
	case  I486_OPCODE_CMP_RM8_FROM_R8:// 0x38,
	case   I486_OPCODE_OR_RM8_FROM_R8:// 0x08,
	case I486_OPCODE_TEST_RM8_FROM_R8:// 0x84,
	case  I486_OPCODE_XOR_RM8_FROM_R8:
		op2.DecodeMODR_MForRegister(8,operand[0]);
		op1.Decode(addressSize,8,operand);
		break;
	case  I486_OPCODE_ADD_RM_FROM_R://   0x01,
	case  I486_OPCODE_AND_RM_FROM_R://   0x21,
	case  I486_OPCODE_CMP_RM_FROM_R://   0x39,
	case   I486_OPCODE_OR_RM_FROM_R://   0x09,
	case I486_OPCODE_TEST_RM_FROM_R://   0x85,
	case I486_OPCODE_XOR_RM_FROM_R:
		op2.DecodeMODR_MForRegister(operandSize,operand[0]);
		op1.Decode(addressSize,operandSize,operand);
		break;
	case I486_OPCODE_ADD_R8_FROM_RM8:// 0x02,
	case I486_OPCODE_AND_R8_FROM_RM8:// 0x22,
	case I486_OPCODE_CMP_R8_FROM_RM8:// 0x3A,
	case  I486_OPCODE_OR_R8_FROM_RM8:// 0x0A,
	case I486_OPCODE_XOR_R8_FROM_RM8:
		op1.DecodeMODR_MForRegister(8,operand[0]);
		op2.Decode(addressSize,8,operand);
		break;
	case I486_OPCODE_ADD_R_FROM_RM://   0x03,
	case I486_OPCODE_AND_R_FROM_RM://   0x23,
	case I486_OPCODE_CMP_R_FROM_RM://   0x3B,
	case  I486_OPCODE_OR_R_FROM_RM://   0x0B,
	case I486_OPCODE_XOR_R_FROM_RM:
		op1.DecodeMODR_MForRegister(operandSize,operand[0]);
		op2.Decode(addressSize,operandSize,operand);
		break;

	}
}

std::string i486DX::Instruction::Disassemble(SegmentRegister cs,unsigned int eip) const
{
	std::string disasm;
	Operand op1,op2;
	std::string op1SizeQual,op2SizeQual;
	std::string op1SegQual,op2SegQual;

	DecodeOperand(addressSize,operandSize,op1,op2);

	switch(opCode)
	{
	case I486_OPCODE_C0_ROL_ROR_RCL_RCR_SAL_SAR_SHL_SHR_RM8_I8://=0xC0,// ::ROL(REG=0),ROR(REG=1),RCL(REG=2),RCR(REG=3),SAL/SHL(REG=4),SHR(REG=5),SAR(REG=7)
	case I486_OPCODE_C1_ROL_ROR_RCL_RCR_SAL_SAR_SHL_SHR_RM_I8:// =0xC1, // ROL(REG=0),ROR(REG=1),RCL(REG=2),RCR(REG=3),SAL/SHL(REG=4),SHR(REG=5),SAR(REG=7)
	case I486_OPCODE_D1_ROL_ROR_RCL_RCR_SAL_SAR_SHL_SHR_RM8_1://=0xD1, // ROL(REG=0),ROR(REG=1),RCL(REG=2),RCR(REG=3),SAL/SHL(REG=4),SHR(REG=5),SAR(REG=7)
		switch(GetREG())
		{
		case 0:
			disasm="ROL";
			break;
		case 1:
			disasm="ROR";
			break;
		case 2:
			disasm="RCL";
			break;
		case 3:
			disasm="RCR";
			break;
		case 4:
			disasm="SHL";
			break;
		case 5:
			disasm="SHR";
			break;
		case 6:
			disasm=cpputil::Ubtox(opCode)+"?";
			break;
		case 7:
			disasm="SAR";
			break;
		}
		switch(opCode)
		{
		case I486_OPCODE_C0_ROL_ROR_RCL_RCR_SAL_SAR_SHL_SHR_RM8_I8://=0xC0,// ::ROL(REG=0),ROR(REG=1),RCL(REG=2),RCR(REG=3),SAL/SHL(REG=4),SHR(REG=5),SAR(REG=7)
			disasm=DisassembleTypicalRM8_I8(disasm,op1,op2);
			break;
		case I486_OPCODE_C1_ROL_ROR_RCL_RCR_SAL_SAR_SHL_SHR_RM_I8:// =0xC1, // ROL(REG=0),ROR(REG=1),RCL(REG=2),RCR(REG=3),SAL/SHL(REG=4),SHR(REG=5),SAR(REG=7)
			disasm=DisassembleTypicalRM_I8(disasm,op1,op2);
			break;
		case I486_OPCODE_D1_ROL_ROR_RCL_RCR_SAL_SAR_SHL_SHR_RM8_1://=0xD1, // ROL(REG=0),ROR(REG=1),RCL(REG=2),RCR(REG=3),SAL/SHL(REG=4),SHR(REG=5),SAR(REG=7)
			disasm=DisassembleTypicalOneOperand(disasm,op1,operandSize)+",1";
			break;
		}
		break;

	case I486_OPCODE_CALL_REL://   0xE8,
	case I486_OPCODE_JMP_REL://          0xE9,   // cw or cd
		disasm=(I486_OPCODE_JMP_REL==opCode ? "JMP" : "CALL");
		cpputil::ExtendString(disasm,8);
		{
			auto offset=GetSimm16or32(operandSize);
			auto destin=eip+offset+numBytes;
			disasm+=cpputil::Uitox(destin);
		}
		break;
	case I486_OPCODE_CALL_FAR://   0x9A,
	case I486_OPCODE_JMP_FAR:
		disasm=(I486_OPCODE_JMP_FAR==opCode ? "JMPF" : "CALLF");
		cpputil::ExtendString(disasm,8);
		disasm+=op1.Disassemble();
		break;


	case I486_OPCODE_CLD:
		disasm="CLD";
		break;
	case I486_OPCODE_CLI:
		disasm="CLI";
		break;


	case I486_OPCODE_FNINIT:
		disasm="FNINIT";
		break;


	case I486_OPCODE_ADD_AL_FROM_I8:
		disasm="ADD     AL,"+op1.Disassemble();
		break;
	case I486_OPCODE_ADD_A_FROM_I:
		if(16==operandSize)
		{
			disasm="ADD     AX,"+op1.Disassemble();;
		}
		else
		{
			disasm="ADD     EAX,"+op1.Disassemble();;
		}
		break;
	case I486_OPCODE_ADD_RM8_FROM_R8:
	case I486_OPCODE_ADD_RM_FROM_R:
	case I486_OPCODE_ADD_R8_FROM_RM8:
	case I486_OPCODE_ADD_R_FROM_RM:
		disasm=DisassembleTypicalTwoOperands("ADD",op1,op2);
		break;


	case I486_OPCODE_AND_AL_FROM_I8:
		disasm="AND     AL,"+op1.Disassemble();
		break;
	case I486_OPCODE_AND_A_FROM_I:
		if(16==operandSize)
		{
			disasm="AND     AX,"+op1.Disassemble();;
		}
		else
		{
			disasm="AND     EAX,"+op1.Disassemble();;
		}
		break;
	case I486_OPCODE_AND_RM8_FROM_R8:
	case I486_OPCODE_AND_RM_FROM_R:
	case I486_OPCODE_AND_R8_FROM_RM8:
	case I486_OPCODE_AND_R_FROM_RM:
		disasm=DisassembleTypicalTwoOperands("AND",op1,op2);
		break;


	case I486_OPCODE_CMP_AL_FROM_I8:
		disasm="CMP     AL,"+op1.Disassemble();
		break;
	case I486_OPCODE_CMP_A_FROM_I:
		if(16==operandSize)
		{
			disasm="CMP     AX,"+op1.Disassemble();;
		}
		else
		{
			disasm="CMP     EAX,"+op1.Disassemble();;
		}
		break;
	case I486_OPCODE_CMP_RM8_FROM_R8:
	case I486_OPCODE_CMP_RM_FROM_R:
	case I486_OPCODE_CMP_R8_FROM_RM8:
	case I486_OPCODE_CMP_R_FROM_RM:
		disasm=DisassembleTypicalTwoOperands("CMP",op1,op2);
		break;


	case I486_OPCODE_TEST_AL_FROM_I8:
		disasm="TEST    AL,"+op1.Disassemble();
		break;
	case I486_OPCODE_TEST_A_FROM_I:
		if(16==operandSize)
		{
			disasm="TEST    AX,"+op1.Disassemble();;
		}
		else
		{
			disasm="TEST    EAX,"+op1.Disassemble();;
		}
		break;
	case I486_OPCODE_TEST_RM8_FROM_R8:
	case I486_OPCODE_TEST_RM_FROM_R:
		disasm=DisassembleTypicalTwoOperands("TEST",op1,op2);
		break;


	case I486_OPCODE_DEC_EAX:
	case I486_OPCODE_DEC_ECX:
	case I486_OPCODE_DEC_EDX:
	case I486_OPCODE_DEC_EBX:
	case I486_OPCODE_DEC_ESP:
	case I486_OPCODE_DEC_EBP:
	case I486_OPCODE_DEC_ESI:
	case I486_OPCODE_DEC_EDI:
		disasm="DEC";
		cpputil::ExtendString(disasm,8);
		if(16==operandSize)
		{
			disasm+=Reg16Str[opCode&7];
		}
		else
		{
			disasm+=Reg32Str[opCode&7];
		}
		break;


	case I486_OPCODE_IN_AL_I8://=        0xE4,
		disasm="IN";
		cpputil::ExtendString(disasm,8);
		disasm+="AL,";
		disasm+=op1.Disassemble();
		break;
	case I486_OPCODE_IN_A_I8://=         0xE5,
		disasm="IN";
		cpputil::ExtendString(disasm,8);
		if(16==operandSize)
		{
			disasm+="AX,";
		}
		else
		{
			disasm+="EAX,";
		}
		disasm+=op1.Disassemble();
		break;
	case I486_OPCODE_IN_AL_DX://=        0xEC,
		disasm="IN      AL,DX";
		break;
	case I486_OPCODE_IN_A_DX://=         0xED,
		if(16==operandSize)
		{
			disasm="IN      AX,DX";
		}
		else
		{
			disasm="IN      EAX,DX";
		}
		break;


	case I486_OPCODE_INC_DEC_R_M8:
		switch(GetREG())
		{
		case 0:
			disasm=DisassembleTypicalOneOperand("INC",op1,8);
			break;
		case 1:
			disasm=DisassembleTypicalOneOperand("DEC",op1,8);
			break;
		}
		break;
	case I486_OPCODE_INC_DEC_CALL_CALLF_JMP_JMPF_PUSH:
		switch(GetREG())
		{
		case 0:
			disasm=DisassembleTypicalOneOperand("INC",op1,operandSize);
			break;
		case 1:
			disasm=DisassembleTypicalOneOperand("DEC",op1,operandSize);
			break;
		case 2:
			disasm=DisassembleTypicalOneOperand("CALL",op1,operandSize);
			break;
		case 3:
			disasm=DisassembleTypicalOneOperand("CALLF",op1,operandSize);
			break;
		case 4:
			disasm=DisassembleTypicalOneOperand("JMP",op1,operandSize);
			break;
		case 5:
			disasm=DisassembleTypicalOneOperand("JMPF",op1,operandSize);
			break;
		case 6:
			disasm=DisassembleTypicalOneOperand("PUSH",op1,operandSize);
			break;
		}
		break;
	case I486_OPCODE_INC_EAX://    0x40, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_ECX://    0x41, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_EDX://    0x42, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_EBX://    0x43, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_ESP://    0x44, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_EBP://    0x45, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_ESI://    0x46, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_EDI://    0x47, // 16/32 depends on OPSIZE_OVERRIDE
		disasm="INC";
		cpputil::ExtendString(disasm,8);
		if(16==operandSize)
		{
			disasm+=Reg16Str[opCode&7];
		}
		else
		{
			disasm+=Reg32Str[opCode&7];
		}
		break;


	case I486_OPCODE_JMP_REL8://         0xEB,   // cb
	case I486_OPCODE_JO_REL8:   // 0x70,
	case I486_OPCODE_JNO_REL8:  // 0x71,
	case I486_OPCODE_JB_REL8:   // 0x72,
	case I486_OPCODE_JAE_REL8:  // 0x73,
	case I486_OPCODE_JE_REL8:   // 0x74,
	case I486_OPCODE_JNE_REL8:  // 0x75,
	case I486_OPCODE_JBE_REL8:  // 0x76,
	case I486_OPCODE_JA_REL8:   // 0x77,
	case I486_OPCODE_JS_REL8:   // 0x78,
	case I486_OPCODE_JNS_REL8:  // 0x79,
	case I486_OPCODE_JP_REL8:   // 0x7A,
	case I486_OPCODE_JNP_REL8:  // 0x7B,
	case I486_OPCODE_JL_REL8:   // 0x7C,
	case I486_OPCODE_JGE_REL8:  // 0x7D,
	case I486_OPCODE_JLE_REL8:  // 0x7E,
	case I486_OPCODE_JG_REL8:   // 0x7F,
		switch(opCode)
		{
		case I486_OPCODE_JMP_REL8://         0xEB,   // cb
			disasm="JMP";
			break;
		case I486_OPCODE_JO_REL8:   // 0x70,
			disasm="JO";
			break;
		case I486_OPCODE_JNO_REL8:  // 0x71,
			disasm="JNO";
			break;
		case I486_OPCODE_JB_REL8:   // 0x72,
			disasm="JB";
			break;
		case I486_OPCODE_JAE_REL8:  // 0x73,
			disasm="JAE";
			break;
		case I486_OPCODE_JE_REL8:   // 0x74,
			disasm="JE";
			break;
		case I486_OPCODE_JNE_REL8:  // 0x75,
			disasm="JNE";
			break;
		case I486_OPCODE_JBE_REL8:  // 0x76,
			disasm="JBE";
			break;
		case I486_OPCODE_JA_REL8:   // 0x77,
			disasm="JA";
			break;
		case I486_OPCODE_JS_REL8:   // 0x78,
			disasm="JS";
			break;
		case I486_OPCODE_JNS_REL8:  // 0x79,
			disasm="JNS";
			break;
		case I486_OPCODE_JP_REL8:   // 0x7A,
			disasm="JP";
			break;
		case I486_OPCODE_JNP_REL8:  // 0x7B,
			disasm="JNP";
			break;
		case I486_OPCODE_JL_REL8:   // 0x7C,
			disasm="JL";
			break;
		case I486_OPCODE_JGE_REL8:  // 0x7D,
			disasm="JGE";
			break;
		case I486_OPCODE_JLE_REL8:  // 0x7E,
			disasm="JLE";
			break;
		case I486_OPCODE_JG_REL8:   // 0x7F,
			disasm="JG";
			break;
		}
		cpputil::ExtendString(disasm,8);
		{
			auto offset=GetSimm8();
			auto destin=eip+offset+numBytes;
			disasm+=cpputil::Uitox(destin);
		}
		break;


	case I486_OPCODE_BINARYOP_RM8_FROM_I8://=  0x80, // AND(REG=4), OR(REG=1), or XOR(REG=6) depends on the REG field of MODR/M
	case I486_OPCODE_BINARYOP_R_FROM_I://=     0x81,
	case I486_OPCODE_BINARYOP_RM_FROM_SXI8://= 0x83,
		switch(GetREG())
		{
		case 0:
			disasm=DisassembleTypicalTwoOperands("ADD",op1,op2);
			break;
		case 1:
			disasm=DisassembleTypicalTwoOperands("OR",op1,op2);
			break;
		case 2:
			disasm=DisassembleTypicalTwoOperands("ADC",op1,op2);
			break;
		case 3:
			disasm=DisassembleTypicalTwoOperands("SBB",op1,op2);
			break;
		case 4:
			disasm=DisassembleTypicalTwoOperands("AND",op1,op2);
			break;
		case 5:
			disasm=DisassembleTypicalTwoOperands("SUB",op1,op2);
			break;
		case 6:
			disasm=DisassembleTypicalTwoOperands("XOR",op1,op2);
			break;
		case 7:
			disasm=DisassembleTypicalTwoOperands("CMP",op1,op2);
			break;
		default:
			disasm=DisassembleTypicalTwoOperands(cpputil::Ubtox(opCode)+"?",op1,op2);
			break;
		}
		break;


	case I486_OPCODE_LGDT_LIDT:
		switch(GetREG())
		{
		case 2:
			disasm=DisassembleTypicalOneOperand("LGDT",op1,16+operandSize);
			break;
		case 3:
			disasm=DisassembleTypicalOneOperand("LIDT",op1,16+operandSize);
			break;
		default:
			disasm=DisassembleTypicalTwoOperands(cpputil::Ubtox(opCode)+"?",op1,op2);
			break;
		}
		break;


	case I486_OPCODE_MOV_FROM_R8: //      0x88,
	case I486_OPCODE_MOV_FROM_R: //       0x89, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_TO_R8: //        0x8A,
	case I486_OPCODE_MOV_TO_R: //         0x8B, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_FROM_SEG: //     0x8C,
	case I486_OPCODE_MOV_TO_SEG: //       0x8E,
	case I486_OPCODE_MOV_M_TO_AL: //      0xA0,
	case I486_OPCODE_MOV_M_TO_EAX: //     0xA1, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_M_FROM_AL: //    0xA2,
	case I486_OPCODE_MOV_M_FROM_EAX: //   0xA3, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I8_TO_AL: //     0xB0,
	case I486_OPCODE_MOV_I8_TO_CL: //     0xB1,
	case I486_OPCODE_MOV_I8_TO_DL: //     0xB2,
	case I486_OPCODE_MOV_I8_TO_BL: //     0xB3,
	case I486_OPCODE_MOV_I8_TO_AH: //     0xB4,
	case I486_OPCODE_MOV_I8_TO_CH: //     0xB5,
	case I486_OPCODE_MOV_I8_TO_DH: //     0xB6,
	case I486_OPCODE_MOV_I8_TO_BH: //     0xB7,
	case I486_OPCODE_MOV_I_TO_EAX: //     0xB8, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_ECX: //     0xB9, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_EDX: //     0xBA, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_EBX: //     0xBB, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_ESP: //     0xBC, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_EBP: //     0xBD, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_ESI: //     0xBE, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_EDI: //     0xBF, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I8_TO_RM8: //    0xC6,
	case I486_OPCODE_MOV_I_TO_RM: //      0xC7, // 16/32 depends on OPSIZE_OVERRIDE

	case I486_OPCODE_MOV_TO_CR://        0x220F,
	case I486_OPCODE_MOV_FROM_CR://      0x200F,
	case I486_OPCODE_MOV_FROM_DR://      0x210F,
	case I486_OPCODE_MOV_TO_DR://        0x230F,
	case I486_OPCODE_MOV_FROM_TR://      0x240F,
	case I486_OPCODE_MOV_TO_TR://        0x260F,

		disasm=DisassembleTypicalTwoOperands("MOV",op1,op2);
		break;


	case I486_OPCODE_MOVZX_R_RM8://=      0xB60F,
		disasm="MOVZX";
		cpputil::ExtendString(disasm,8);
		disasm+=op1.Disassemble();
		disasm.push_back(',');
		disasm+=Operand::GetSizeQualifierToDisassembly(op2,8);
		disasm+=op2.Disassemble();
		break;
	case I486_OPCODE_MOVZX_R32_RM16://=   0xB70F,
		disasm="MOVZX";
		cpputil::ExtendString(disasm,8);
		disasm+=op1.Disassemble();
		disasm.push_back(',');
		disasm+=Operand::GetSizeQualifierToDisassembly(op2,16);
		disasm+=op2.Disassemble();
		break;


	case I486_OPCODE_RET://              0xC3,
		disasm="RET";
		break;
	case I486_OPCODE_RETF://             0xCB,
		disasm="RETF";
		break;
	case I486_OPCODE_RET_I16://          0xC2,
		disasm="RET";
		cpputil::ExtendString(disasm,8);
		disasm+=op1.Disassemble();
		break;
	case I486_OPCODE_RETF_I16://         0xCA,
		disasm="RETF";
		cpputil::ExtendString(disasm,8);
		disasm+=op1.Disassemble();
		break;




	case I486_OPCODE_OUT_I8_AL: //        0xE6,
		disasm="OUT";
		cpputil::ExtendString(disasm,8);
		disasm+=op1.Disassemble();
		disasm+=",AL";
		break;
	case I486_OPCODE_OUT_I8_A: //         0xE7,
		disasm="OUT";
		cpputil::ExtendString(disasm,8);
		disasm+=op1.Disassemble();
		if(16==operandSize)
		{
			disasm+=",AX";
		}
		else
		{
			disasm+=",EAX";
		}
		break;
	case I486_OPCODE_OUT_DX_AL: //        0xEE,
		disasm="OUT     DX,AL";
		break;
	case I486_OPCODE_OUT_DX_A: //         0xEF,
		if(16==operandSize)
		{
			disasm="OUT     DX,AX";
		}
		else
		{
			disasm="OUT     DX,EAX";
		}
		break;


	case I486_OPCODE_PUSHA://            0x60,
		if(16==operandSize)
		{
			disasm="PUSHA";
		}
		else
		{
			disasm="PUSHAD";
		}
		break;
	case I486_OPCODE_PUSHF://            0x9C,
		if(16==operandSize)
		{
			disasm="PUSHF";
		}
		else
		{
			disasm="PUSHFD";
		}
		break;


	case I486_OPCODE_PUSH_EAX://         0x50,
	case I486_OPCODE_PUSH_ECX://         0x51,
	case I486_OPCODE_PUSH_EDX://         0x52,
	case I486_OPCODE_PUSH_EBX://         0x53,
	case I486_OPCODE_PUSH_ESP://         0x54,
	case I486_OPCODE_PUSH_EBP://         0x55,
	case I486_OPCODE_PUSH_ESI://         0x56,
	case I486_OPCODE_PUSH_EDI://         0x57,
		if(16==operandSize)
		{
			disasm="PUSH    ";
			disasm+=Reg16Str[opCode&7];
		}
		else
		{
			disasm="PUSH    ";
			disasm+=Reg32Str[opCode&7];
		}
		break;
	case I486_OPCODE_PUSH_I8://          0x6A,
		disasm=DisassembleTypicalOneOperand("PUSH",op1,8);
		break;
	case I486_OPCODE_PUSH_I://           0x68,
		disasm=DisassembleTypicalOneOperand("PUSH",op1,operandSize);
		break;
	case I486_OPCODE_PUSH_CS://          0x0E,
		disasm="PUSH    CS";
		break;
	case I486_OPCODE_PUSH_SS://          0x16,
		disasm="PUSH    SS";
		break;
	case I486_OPCODE_PUSH_DS://          0x1E,
		disasm="PUSH    DS";
		break;
	case I486_OPCODE_PUSH_ES://          0x06,
		disasm="PUSH    ES";
		break;
	case I486_OPCODE_PUSH_FS://          0xA00F,
		disasm="PUSH    FS";
		break;
	case I486_OPCODE_PUSH_GS://          0xA80F,
		disasm="PUSH    GS";
		break;


	case I486_OPCODE_POP_M://            0x8F,
		disasm=DisassembleTypicalOneOperand("POP",op1,operandSize);
		break;
	case I486_OPCODE_POP_EAX://          0x58,
	case I486_OPCODE_POP_ECX://          0x59,
	case I486_OPCODE_POP_EDX://          0x5A,
	case I486_OPCODE_POP_EBX://          0x5B,
	case I486_OPCODE_POP_ESP://          0x5C,
	case I486_OPCODE_POP_EBP://          0x5D,
	case I486_OPCODE_POP_ESI://          0x5E,
	case I486_OPCODE_POP_EDI://          0x5F,
		if(16==operandSize)
		{
			disasm="POP     ";
			disasm+=Reg16Str[opCode&7];
		}
		else
		{
			disasm="POP     ";
			disasm+=Reg32Str[opCode&7];
		}
		break;
	case I486_OPCODE_POP_SS://           0x17,
		disasm="POP     SS";
		break;
	case I486_OPCODE_POP_DS://           0x1F,
		disasm="POP     DS";
		break;
	case I486_OPCODE_POP_ES://           0x07,
		disasm="POP     ES";
		break;
	case I486_OPCODE_POP_FS://           0xA10F,
		disasm="POP     FS";
		break;
	case I486_OPCODE_POP_GS://           0xA90F,
		disasm="POP     GS";
		break;


	case I486_OPCODE_POPA://             0x61,
		switch(operandSize)
		{
		case 16:
			disasm="POPA";
			break;
		case 32:
			disasm="POPAD";
			break;
		}
		break;
	case I486_OPCODE_POPF://             0x9D,
		switch(operandSize)
		{
		case 16:
			disasm="POPF";
			break;
		case 32:
			disasm="POPFD";
			break;
		}
		break;


	case I486_OPCODE_OR_AL_FROM_I8:
		disasm="OR      AL,"+op1.Disassemble();
		break;
	case I486_OPCODE_OR_A_FROM_I:
		if(16==operandSize)
		{
			disasm="OR      AX,"+op1.Disassemble();;
		}
		else
		{
			disasm="OR      EAX,"+op1.Disassemble();;
		}
		break;
	case I486_OPCODE_OR_RM8_FROM_R8:
	case I486_OPCODE_OR_RM_FROM_R:
	case I486_OPCODE_OR_R8_FROM_RM8:
	case I486_OPCODE_OR_R_FROM_RM:
		disasm=DisassembleTypicalTwoOperands("OR",op1,op2);
		break;


	case I486_OPCODE_XOR_AL_FROM_I8:
		disasm="XOR     AL,"+op1.Disassemble();
		break;
	case I486_OPCODE_XOR_A_FROM_I:
		if(16==operandSize)
		{
			disasm="XOR     AX,"+op1.Disassemble();;
		}
		else
		{
			disasm="XOR     EAX,"+op1.Disassemble();;
		}
		break;
	case I486_OPCODE_XOR_RM8_FROM_R8:
	case I486_OPCODE_XOR_RM_FROM_R:
	case I486_OPCODE_XOR_R8_FROM_RM8:
	case I486_OPCODE_XOR_R_FROM_RM:
		disasm=DisassembleTypicalTwoOperands("XOR",op1,op2);
		break;
	}

	return disasm;
}

std::string i486DX::Instruction::DisassembleTypicalOneOperand(std::string inst,const Operand &op,int operandSize) const
{
	auto sizeQual=i486DX::Operand::GetSizeQualifierToDisassembly(op,operandSize);
	auto segQual=i486DX::Operand::GetSegmentQualifierToDisassembly(segOverride,op);
	auto disasm=inst;
	cpputil::ExtendString(disasm,8);
	disasm+=sizeQual+segQual+op.Disassemble();
	return disasm;
}

std::string i486DX::Instruction::DisassembleTypicalRM8_I8(std::string inst,const Operand &op1,const Operand &op2) const
{
	auto sizeQual=i486DX::Operand::GetSizeQualifierToDisassembly(op1,8);
	auto segQual=i486DX::Operand::GetSegmentQualifierToDisassembly(segOverride,op1);
	auto disasm=inst;
	cpputil::ExtendString(disasm,8);
	disasm+=sizeQual+segQual+op1.Disassemble()+","+op2.Disassemble();
	return disasm;
}

std::string i486DX::Instruction::DisassembleTypicalRM_I8(std::string inst,const Operand &op1,const Operand &op2) const
{
	auto sizeQual=i486DX::Operand::GetSizeQualifierToDisassembly(op1,operandSize);
	auto segQual=i486DX::Operand::GetSegmentQualifierToDisassembly(segOverride,op1);
	auto disasm=inst;
	cpputil::ExtendString(disasm,8);
	disasm+=sizeQual+segQual+op1.Disassemble()+","+op2.Disassemble();
	return disasm;
}

std::string i486DX::Instruction::DisassembleTypicalTwoOperands(std::string inst,const Operand &op1,const Operand &op2) const
{
	std::string disasm=inst,op1SizeQual,op2SizeQual,op1SegQual,op2SegQual;
	cpputil::ExtendString(disasm,8);

	i486DX::Operand::GetSizeQualifierToDisassembly(op1SizeQual,op2SizeQual,op1,op2);
	op1SegQual=i486DX::Operand::GetSegmentQualifierToDisassembly(segOverride,op1);
	op2SegQual=i486DX::Operand::GetSegmentQualifierToDisassembly(segOverride,op2);
	disasm+=op1SizeQual+op1SegQual+op1.Disassemble();
	disasm.push_back(',');
	disasm+=op2SizeQual+op2SegQual+op2.Disassemble();

	return disasm;
}

unsigned int i486DX::Instruction::GetUimm8(void) const
{
	return operand[operandLen-1];
}
unsigned int i486DX::Instruction::GetUimm16(void) const
{
	return cpputil::GetWord(operand+operandLen-2);
}
unsigned int i486DX::Instruction::GetUimm32(void) const
{
	return cpputil::GetDword(operand+operandLen-4);
}
unsigned int i486DX::Instruction::GetUimm16or32(unsigned int operandSize) const
{
	if(16==operandSize)
	{
		return GetUimm16();
	}
	else
	{
		return GetUimm32();
	}
}
int i486DX::Instruction::GetSimm8(void) const
{
	return cpputil::GetSignedByte(operand[operandLen-1]);
}
int i486DX::Instruction::GetSimm16(void) const
{
	return cpputil::GetSignedWord(operand+operandLen-2);
}
int i486DX::Instruction::GetSimm32(void) const
{
	return cpputil::GetSignedDword(operand+operandLen-4);
}
int i486DX::Instruction::GetSimm16or32(unsigned int operandSize) const
{
	if(16==operandSize)
	{
		return GetSimm16();
	}
	else
	{
		return GetSimm32();
	}
}

/* static */ std::string i486DX::Get8BitRegisterNameFromMODR_M(unsigned char MOD_RM)
{
	auto REG_OPCODE=((MOD_RM>>3)&7);
	return Reg8Str[REG_OPCODE];
}
/* static */ std::string i486DX::Get16BitRegisterNameFromMODR_M(unsigned char MOD_RM)
{
	auto REG_OPCODE=((MOD_RM>>3)&7);
	return Reg16Str[REG_OPCODE];
}
/* static */ std::string i486DX::Get32BitRegisterNameFromMODR_M(unsigned char MOD_RM)
{
	auto REG_OPCODE=((MOD_RM>>3)&7);
	return Reg32Str[REG_OPCODE];
}
/* static */ std::string i486DX::Get16or32BitRegisterNameFromMODR_M(int dataSize,unsigned char MOD_RM)
{
	if(16==dataSize)
	{
		auto REG_OPCODE=((MOD_RM>>3)&7);
		return Reg16Str[REG_OPCODE];
	}
	else
	{
		auto REG_OPCODE=((MOD_RM>>3)&7);
		return Reg32Str[REG_OPCODE];
	}
}


unsigned int i486DX::RunOneInstruction(Memory &mem,InOut &io)
{
	state.holdIRQ=false;

	auto inst=FetchInstruction(state.CS(),state.EIP,mem);

	Operand op1,op2;
	inst.DecodeOperand(inst.addressSize,inst.operandSize,op1,op2);

	bool EIPChanged=false;
	unsigned int clocksPassed=0;

	switch(inst.opCode)
	{
	case I486_OPCODE_C0_ROL_ROR_RCL_RCR_SAL_SAR_SHL_SHR_RM8_I8://=0xC0,// ::ROL(REG=0),ROR(REG=1),RCL(REG=2),RCR(REG=3),SAL/SHL(REG=4),SHR(REG=5),SAR(REG=7)
		{
			auto value=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op1,1);
			auto i=value.GetAsDword();
			auto ctr=inst.GetUimm8()&31; // [1] pp.26-243 Only bottom 5 bits are used.
			if(true==state.exception)
			{
				break;
			}
			switch(inst.GetREG())
			{
			case 0:// "ROL";
				Abort("C1 ROL not implemented yet.");
				clocksPassed=(OPER_ADDR==op1.operandType ? 4 : 2);
				break;
			case 1:// "ROR";
				Abort("C1 ROR not implemented yet.");
				clocksPassed=(OPER_ADDR==op1.operandType ? 4 : 2);
				break;
			case 2:// "RCL";
				Abort("C1 RCL not implemented yet.");
				clocksPassed=(OPER_ADDR==op1.operandType ? 10 : 11);  // See reminder #20200123-1
				break;
			case 3:// "RCR";
				Abort("C1 RCR not implemented yet.");
				clocksPassed=(OPER_ADDR==op1.operandType ? 10 : 11);  // See reminder #20200123-1
				break;
			case 4:// "SHL";
				ShlByte(i,ctr);
				clocksPassed=(OPER_ADDR==op1.operandType ? 4 : 2);
				break;
			case 5:// "SHR";
				ShrByte(i,ctr);
				clocksPassed=(OPER_ADDR==op1.operandType ? 4 : 2);
				break;
			case 6:// cpputil::Ubtox(opCode)+"?";
				Abort("Undefined REG for "+cpputil::Ubtox(inst.opCode));
				clocksPassed=(OPER_ADDR==op1.operandType ? 4 : 2);
				break;
			case 7:// "SAR";
				Abort("C1 SAR not implemented yet.");
				clocksPassed=(OPER_ADDR==op1.operandType ? 4 : 2);
				break;
			}
			value.SetDword(i);
			StoreOperandValue(op1,mem,inst.addressSize,inst.segOverride,value);
		}
		break;
	case I486_OPCODE_C1_ROL_ROR_RCL_RCR_SAL_SAR_SHL_SHR_RM_I8:// =0xC1, // ROL(REG=0),ROR(REG=1),RCL(REG=2),RCR(REG=3),SAL/SHL(REG=4),SHR(REG=5),SAR(REG=7)
		{
			auto value=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op1,1);
			auto i=value.GetAsDword();
			auto ctr=inst.GetUimm8()&31; // [1] pp.26-243 Only bottom 5 bits are used.
			if(true==state.exception)
			{
				break;
			}
			switch(inst.GetREG())
			{
			case 0:// "ROL";
				Abort("C1 ROL not implemented yet.");
				clocksPassed=(OPER_ADDR==op1.operandType ? 4 : 2);
				break;
			case 1:// "ROR";
				Abort("C1 ROR not implemented yet.");
				clocksPassed=(OPER_ADDR==op1.operandType ? 4 : 2);
				break;
			case 2:// "RCL";
				Abort("C1 RCL not implemented yet.");
				clocksPassed=(OPER_ADDR==op1.operandType ? 10 : 11);  // See reminder #20200123-1
				break;
			case 3:// "RCR";
				Abort("C1 RCR not implemented yet.");
				clocksPassed=(OPER_ADDR==op1.operandType ? 10 : 11);  // See reminder #20200123-1
				break;
			case 4:// "SHL";
				ShlWordOrDword(inst.operandSize,i,ctr);
				clocksPassed=(OPER_ADDR==op1.operandType ? 4 : 2);
				break;
			case 5:// "SHR";
				ShrWordOrDword(inst.operandSize,i,ctr);
				clocksPassed=(OPER_ADDR==op1.operandType ? 4 : 2);
				break;
			case 6:// cpputil::Ubtox(opCode)+"?";
				Abort("Undefined REG for "+cpputil::Ubtox(inst.opCode));
				clocksPassed=(OPER_ADDR==op1.operandType ? 4 : 2);
				break;
			case 7:// "SAR";
				Abort("C1 SAR not implemented yet.");
				clocksPassed=(OPER_ADDR==op1.operandType ? 4 : 2);
				break;
			}
			value.SetDword(i);
			StoreOperandValue(op1,mem,inst.addressSize,inst.segOverride,value);
		}
		break;
	case I486_OPCODE_D1_ROL_ROR_RCL_RCR_SAL_SAR_SHL_SHR_RM8_1://=0xD1, // ROL(REG=0),ROR(REG=1),RCL(REG=2),RCR(REG=3),SAL/SHL(REG=4),SHR(REG=5),SAR(REG=7)
		{
			auto value=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op1,1);
			auto i=value.GetAsDword();
			if(true==state.exception)
			{
				break;
			}
			switch(inst.GetREG())
			{
			case 0:// "ROL";
				Abort("C1 ROL not implemented yet.");
				clocksPassed=(OPER_ADDR==op1.operandType ? 4 : 2);
				break;
			case 1:// "ROR";
				Abort("C1 ROR not implemented yet.");
				clocksPassed=(OPER_ADDR==op1.operandType ? 4 : 2);
				break;
			case 2:// "RCL";
				Abort("C1 RCL not implemented yet.");
				clocksPassed=(OPER_ADDR==op1.operandType ? 10 : 11);  // See reminder #20200123-1
				break;
			case 3:// "RCR";
				Abort("C1 RCR not implemented yet.");
				clocksPassed=(OPER_ADDR==op1.operandType ? 10 : 11);  // See reminder #20200123-1
				break;
			case 4:// "SHL";
				ShlWordOrDword(inst.operandSize,i,1);
				clocksPassed=(OPER_ADDR==op1.operandType ? 4 : 2);
				break;
			case 5:// "SHR";
				ShrWordOrDword(inst.operandSize,i,1);
				clocksPassed=(OPER_ADDR==op1.operandType ? 4 : 2);
				break;
			case 6:// cpputil::Ubtox(opCode)+"?";
				Abort("Undefined REG for "+cpputil::Ubtox(inst.opCode));
				clocksPassed=(OPER_ADDR==op1.operandType ? 4 : 2);
				break;
			case 7:// "SAR";
				Abort("C1 SAR not implemented yet.");
				clocksPassed=(OPER_ADDR==op1.operandType ? 4 : 2);
				break;
			}
			value.SetDword(i);
			StoreOperandValue(op1,mem,inst.addressSize,inst.segOverride,value);
		}
		break;


	case I486_OPCODE_F6_TEST_NOT_NEG_MUL_IMUL_DIV_IDIV: //=0xF6
	case I486_OPCODE_F7_TEST_NOT_NEG_MUL_IMUL_DIV_IDIV: //=0xF7,
		Abort("F6 and F7 not implemented yet.");
		break;


	case  I486_OPCODE_ADD_AL_FROM_I8://  0x04,
	case  I486_OPCODE_AND_AL_FROM_I8://  0x24,
	case  I486_OPCODE_CMP_AL_FROM_I8://  0x3C,
	case   I486_OPCODE_OR_AL_FROM_I8://  0x0C,
	case I486_OPCODE_TEST_AL_FROM_I8://  0xA8,
	case  I486_OPCODE_XOR_AL_FROM_I8:
		{
			clocksPassed=1;
			auto value=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op1,1);
			if(true==state.exception)
			{
				break;
			}
			auto al=GetAL();
			auto v=value.GetAsDword();
			switch(inst.opCode)
			{
			case I486_OPCODE_ADD_AL_FROM_I8://  0x04,
				AddByte(al,v);
				break;
			case I486_OPCODE_AND_AL_FROM_I8://  0x24,
			case I486_OPCODE_TEST_AL_FROM_I8://  0xA8,
				AndByte(al,v);
				break;
			case I486_OPCODE_CMP_AL_FROM_I8://  0x3C,
				SubByte(al,v);
				break;
			case I486_OPCODE_OR_AL_FROM_I8://    0x0C,
				OrByte(al,v);
				break;
			case I486_OPCODE_XOR_AL_FROM_I8:
				XorByte(al,v);
				break;
			}
			if(I486_OPCODE_TEST_AL_FROM_I8!=inst.opCode &&
			   I486_OPCODE_CMP_AL_FROM_I8!=inst.opCode) // Don't actually update value if TEST or CMP.
			{
				SetAL(al);
			}
		}
		break;
	case  I486_OPCODE_ADD_A_FROM_I://    0x05,
	case  I486_OPCODE_AND_A_FROM_I://    0x25,
	case  I486_OPCODE_CMP_A_FROM_I://    0x3D,
	case   I486_OPCODE_OR_A_FROM_I://    0x0D,
	case I486_OPCODE_TEST_A_FROM_I://    0xA9,
	case  I486_OPCODE_XOR_A_FROM_I:
		clocksPassed=1;
		if(16==inst.operandSize)
		{
			auto value=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op1,inst.operandSize/8);
			if(true==state.exception)
			{
				break;
			}
			auto ax=GetAX();
			auto v=value.GetAsDword();
			switch(inst.opCode)
			{
			case I486_OPCODE_ADD_A_FROM_I://    0x05,
				AddWord(ax,v);
				break;
			case I486_OPCODE_AND_A_FROM_I://    0x25,
			case I486_OPCODE_TEST_A_FROM_I://    0xA9,
				AndWord(ax,v);
				break;
			case I486_OPCODE_CMP_A_FROM_I://    0x3D,
				SubWord(ax,v);
				break;
			case I486_OPCODE_OR_A_FROM_I://      0x0D,
				OrWord(ax,v);
				break;
			case I486_OPCODE_XOR_A_FROM_I:
				XorWord(ax,v);
				break;
			}
			if(I486_OPCODE_TEST_A_FROM_I!=inst.opCode &&
			   I486_OPCODE_CMP_A_FROM_I!=inst.opCode)
			{
				SetAX(ax);
			}
		}
		else
		{
			auto value=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op1,inst.operandSize/8);
			if(true==state.exception)
			{
				break;
			}
			auto eax=GetEAX();
			auto v=value.GetAsDword();
			switch(inst.opCode)
			{
			case I486_OPCODE_AND_A_FROM_I://    0x25,
			case I486_OPCODE_TEST_A_FROM_I://    0xA9,
				AndDword(eax,v);
				break;
			case I486_OPCODE_CMP_A_FROM_I://    0x3D,
				SubDword(eax,v);
				break;
			case I486_OPCODE_OR_A_FROM_I://      0x0D,
				OrDword(eax,v);
				break;
			case I486_OPCODE_XOR_A_FROM_I:
				XorDword(eax,v);
				break;
			}
			if(I486_OPCODE_TEST_A_FROM_I!=inst.opCode &&
			   I486_OPCODE_CMP_A_FROM_I!=inst.opCode)
			{
				SetEAX(eax);
			}
		}
		break;
	case  I486_OPCODE_ADD_RM8_FROM_R8:// 0x00,
	case  I486_OPCODE_AND_RM8_FROM_R8:// 0x20,
	case  I486_OPCODE_CMP_RM8_FROM_R8:// 0x38,
	case   I486_OPCODE_OR_RM8_FROM_R8:// 0x08,
	case  I486_OPCODE_XOR_RM8_FROM_R8:
	case I486_OPCODE_TEST_RM8_FROM_R8:// 0x84,

	case I486_OPCODE_ADD_R8_FROM_RM8:// 0x02,
	case I486_OPCODE_AND_R8_FROM_RM8:// 0x22,
	case I486_OPCODE_CMP_R8_FROM_RM8:// 0x3A,
	case  I486_OPCODE_OR_R8_FROM_RM8:// 0x0A,
	case I486_OPCODE_XOR_R8_FROM_RM8:
		{
			if(op1.operandType==OPER_ADDR || op2.operandType==OPER_ADDR)
			{
				if(I486_OPCODE_TEST_RM8_FROM_R8!=inst.opCode)
				{
					clocksPassed=3;
				}
				else
				{
					clocksPassed=2;
				}
			}
			else
			{
				clocksPassed=1;
			}

			auto value1=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op1,1);
			auto value2=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op2,1);
			if(true==state.exception)
			{
				break;
			}
			auto i=value1.GetAsDword();
			switch(inst.opCode)
			{
			case I486_OPCODE_ADD_RM8_FROM_R8:// 0x00,
			case I486_OPCODE_ADD_R8_FROM_RM8:// 0x02,
				AddByte(i,value2.GetAsDword());
				break;
			case I486_OPCODE_TEST_RM8_FROM_R8:// 0x84,
			case I486_OPCODE_AND_RM8_FROM_R8:// 0x20,
			case I486_OPCODE_AND_R8_FROM_RM8:// 0x22,
				AndByte(i,value2.GetAsDword());
				break;
			case I486_OPCODE_CMP_RM8_FROM_R8:// 0x38,
			case I486_OPCODE_CMP_R8_FROM_RM8:// 0x3A,
				SubByte(i,value2.GetAsDword());
				break;
			case I486_OPCODE_OR_RM8_FROM_R8://   0x08,
			case I486_OPCODE_OR_R8_FROM_RM8://   0x0A,
				OrByte(i,value2.GetAsDword());
				break;
			case I486_OPCODE_XOR_RM8_FROM_R8:
			case I486_OPCODE_XOR_R8_FROM_RM8:
				XorByte(i,value2.GetAsDword());
				break;
			}
			if(I486_OPCODE_TEST_RM8_FROM_R8!=inst.opCode &&
			   I486_OPCODE_CMP_RM8_FROM_R8!=inst.opCode &&
			   I486_OPCODE_CMP_R8_FROM_RM8!=inst.opCode)
			{
				value1.SetDword(i);
				StoreOperandValue(op1,mem,inst.addressSize,inst.segOverride,value1);
			}
		}
		break;
	case  I486_OPCODE_ADD_RM_FROM_R://   0x01,
	case  I486_OPCODE_AND_RM_FROM_R://   0x21,
	case  I486_OPCODE_CMP_RM_FROM_R://   0x39,
	case I486_OPCODE_TEST_RM_FROM_R://   0x85,
	case   I486_OPCODE_OR_RM_FROM_R://   0x09,
	case  I486_OPCODE_XOR_RM_FROM_R:

	case I486_OPCODE_ADD_R_FROM_RM://    0x03,
	case I486_OPCODE_AND_R_FROM_RM://    0x23,
	case I486_OPCODE_CMP_R_FROM_RM://    0x3B,
	case  I486_OPCODE_OR_R_FROM_RM://    0x0B,
	case I486_OPCODE_XOR_R_FROM_RM:
		{
			if(op1.operandType==OPER_ADDR || op2.operandType==OPER_ADDR)
			{
				if(I486_OPCODE_TEST_RM_FROM_R!=inst.opCode)
				{
					clocksPassed=3;
				}
				else
				{
					clocksPassed=2;
				}
			}
			else
			{
				clocksPassed=1;
			}

			auto value1=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op1,inst.operandSize/8);
			auto value2=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op2,inst.operandSize/8);
			if(true==state.exception)
			{
				break;
			}
			auto i=value1.GetAsDword();
			switch(inst.opCode)
			{
			case I486_OPCODE_ADD_RM_FROM_R://   0x01,
			case I486_OPCODE_ADD_R_FROM_RM://   0x03,
				AddWordOrDword(inst.operandSize,i,value2.GetAsDword());
				break;
			case  I486_OPCODE_AND_RM_FROM_R://   0x21,
			case I486_OPCODE_TEST_RM_FROM_R://   0x85,
			case  I486_OPCODE_AND_R_FROM_RM://   0x23,
				AndWordOrDword(inst.operandSize,i,value2.GetAsDword());
				break;
			case I486_OPCODE_CMP_RM_FROM_R://   0x39,
			case I486_OPCODE_CMP_R_FROM_RM://   0x3B,
				SubWordOrDword(inst.operandSize,i,value2.GetAsDword());
				break;
			case I486_OPCODE_OR_RM_FROM_R://     0x09,
			case I486_OPCODE_OR_R_FROM_RM://     0x0B,
				OrWordOrDword(inst.operandSize,i,value2.GetAsDword());
				break;
			case I486_OPCODE_XOR_RM_FROM_R:
			case I486_OPCODE_XOR_R_FROM_RM:
				XorWordOrDword(inst.operandSize,i,value2.GetAsDword());
				break;
			}
			if(I486_OPCODE_TEST_RM_FROM_R!=inst.opCode &&
			   I486_OPCODE_CMP_RM_FROM_R!=inst.opCode &&
			   I486_OPCODE_CMP_R_FROM_RM!=inst.opCode)
			{
				value1.SetDword(i);
				StoreOperandValue(op1,mem,inst.addressSize,inst.segOverride,value1);
			}
		}
		break;


	case I486_OPCODE_CALL_FAR://   0x9A,
		{
			if(true==IsInRealMode())
			{
				clocksPassed=18;
			}
			else
			{
				clocksPassed=20;
			}
			Push(mem,inst.operandSize,state.CS().value);
			Push(mem,inst.operandSize,state.EIP+inst.numBytes);
			LoadSegmentRegister(state.CS(),op1.seg,mem);
			state.EIP=op1.offset;
			EIPChanged=true;
		}
		break;
	case I486_OPCODE_CALL_REL://   0xE8,
	case I486_OPCODE_JMP_REL://          0xE9,   // cw or cd
		{
			clocksPassed=3;
			if(I486_OPCODE_CALL_REL==inst.opCode)
			{
				Push(mem,inst.operandSize,state.EIP+inst.numBytes);
			}
			auto offset=inst.GetSimm16or32(inst.operandSize);
			auto destin=state.EIP+offset+inst.numBytes;
			if(16==inst.operandSize)
			{
				destin&=0xffff;
			}
			state.EIP=destin;
			EIPChanged=true;
		}
		break;


	case I486_OPCODE_CLD:
		state.EFLAGS&=(~EFLAGS_DIRECTION);
		clocksPassed=2;
		break;
	case I486_OPCODE_CLI:
		state.EFLAGS&=(~EFLAGS_INT_ENABLE);
		clocksPassed=2;
		break;


	case I486_OPCODE_FNINIT:
		clocksPassed=17;
		break;


	case I486_OPCODE_DEC_EAX:
	case I486_OPCODE_DEC_ECX:
	case I486_OPCODE_DEC_EDX:
	case I486_OPCODE_DEC_EBX:
	case I486_OPCODE_DEC_ESP:
	case I486_OPCODE_DEC_EBP:
	case I486_OPCODE_DEC_ESI:
	case I486_OPCODE_DEC_EDI:
		{
			auto value=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op1,op1.GetSize());
			if(true!=state.exception)
			{
				auto i=value.GetAsDword();
				DecrementWordOrDword(inst.operandSize,i);
				value.SetDword(i);
				StoreOperandValue(op1,mem,inst.addressSize,inst.segOverride,value);
			}
		}
		clocksPassed=1;
		break;


	case I486_OPCODE_IN_AL_I8://=        0xE4,
		{
			auto ioRead=io.In8(inst.operand[0]);
			if(true!=state.exception)
			{
				SetAL(ioRead);
			}
		}
		if(true==IsInRealMode())
		{
			clocksPassed=14;
		}
		else
		{
			clocksPassed=8; // 28 if CPL>IOPL
		}
		break;
	case I486_OPCODE_IN_A_I8://=         0xE5,
		if(16==inst.operandSize)
		{
			auto ioRead=io.In16(inst.operand[0]);
			if(true!=state.exception)
			{
				SetAX(ioRead);
			}
		}
		else
		{
			auto ioRead=io.In32(inst.operand[0]);
			if(true!=state.exception)
			{
				SetEAX(ioRead);
			}
		}
		if(true==IsInRealMode())
		{
			clocksPassed=14;
		}
		else
		{
			clocksPassed=8; // 28 if CPL>IOPL
		}
		break;
	case I486_OPCODE_IN_AL_DX://=        0xEC,
		{
			auto ioRead=io.In8(GetDX());
			if(true!=state.exception)
			{
				SetAL(ioRead);
			}
		}
		if(true==IsInRealMode())
		{
			clocksPassed=14;
		}
		else
		{
			clocksPassed=8; // 28 if CPL>IOPL
		}
		break;
	case I486_OPCODE_IN_A_DX://=         0xED,
		if(16==inst.operandSize)
		{
			auto ioRead=io.In16(GetDX());
			if(true!=state.exception)
			{
				SetAX(ioRead);
			}
		}
		else
		{
			auto ioRead=io.In32(GetDX());
			if(true!=state.exception)
			{
				SetEAX(ioRead);
			}
		}
		if(true==IsInRealMode())
		{
			clocksPassed=14;
		}
		else
		{
			clocksPassed=8; // 28 if CPL>IOPL
		}
		break;


	case I486_OPCODE_INC_DEC_R_M8:
		{
			auto value=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op1,1);
			if(true!=state.exception)
			{
				auto i=value.GetAsDword();
				switch(inst.GetREG())
				{
				case 0:
					IncrementByte(i);
					break;
				case 1:
					DecrementByte(i);
					break;
				default:
					Abort("Undefined REG for "+cpputil::Ubtox(inst.opCode));
					break;
				}
				value.SetDword(i);
				StoreOperandValue(op1,mem,inst.addressSize,inst.segOverride,value);
			}
			if(op1.operandType==OPER_ADDR)
			{
				clocksPassed=3;
			}
			else
			{
				clocksPassed=1;
			}
		}
		break;
	case I486_OPCODE_INC_DEC_CALL_CALLF_JMP_JMPF_PUSH:
		{
			auto REG=inst.GetREG();
			switch(REG)
			{
			case 0:
			case 1:
				{
					auto value=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op1,inst.operandSize/8);
					if(true!=state.exception)
					{
						auto i=value.GetAsDword();
						if(0==REG)
						{
							IncrementWordOrDword(inst.operandSize,i);
						}
						else
						{
							DecrementWordOrDword(inst.operandSize,i);
						}
						value.SetDword(i);
						StoreOperandValue(op1,mem,inst.addressSize,inst.segOverride,value);
					}
					if(op1.operandType==OPER_ADDR)
					{
						clocksPassed=3;
					}
					else
					{
						clocksPassed=1;
					}
				}
				break;
			case 5: // JMPF Indirect
				{
					auto value=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op1,(inst.operandSize+16)/8);
					if(true!=state.exception)
					{
						SetIPorEIP(inst.operandSize,value.GetAsDword());
						LoadSegmentRegister(state.CS(),value.GetFwordSegment(),mem);
						EIPChanged=true;
					}
					if(op1.operandType==OPER_ADDR)
					{
						clocksPassed=3;
					}
					else
					{
						clocksPassed=1;
					}
				}
				break;
			case 6: // PUSH
				{
					auto value=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op1,inst.operandSize/8);
					clocksPassed=4;
					if(true!=state.exception)
					{
						Push(mem,inst.operandSize,value.GetAsDword());
					}
				}
				break;
			default:
				Abort("Undefined REG for "+cpputil::Ubtox(inst.opCode));
				break;
			}
		}
		break;
	case I486_OPCODE_INC_EAX://    0x40, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_ECX://    0x41, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_EDX://    0x42, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_EBX://    0x43, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_ESP://    0x44, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_EBP://    0x45, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_ESI://    0x46, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_INC_EDI://    0x47, // 16/32 depends on OPSIZE_OVERRIDE
		{
			auto value=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op1,op1.GetSize());
			if(true!=state.exception)
			{
				auto i=value.GetAsDword();
				IncrementWordOrDword(inst.operandSize,i);
				value.SetDword(i);
				StoreOperandValue(op1,mem,inst.addressSize,inst.segOverride,value);
			}
		}
		clocksPassed=1;


	case I486_OPCODE_JMP_REL8://         0xEB,   // cb
	case I486_OPCODE_JO_REL8:   // 0x70,
	case I486_OPCODE_JNO_REL8:  // 0x71,
	case I486_OPCODE_JB_REL8:   // 0x72,
	case I486_OPCODE_JAE_REL8:  // 0x73,
	case I486_OPCODE_JE_REL8:   // 0x74,
	case I486_OPCODE_JNE_REL8:  // 0x75,
	case I486_OPCODE_JBE_REL8:  // 0x76,
	case I486_OPCODE_JA_REL8:   // 0x77,
	case I486_OPCODE_JS_REL8:   // 0x78,
	case I486_OPCODE_JNS_REL8:  // 0x79,
	case I486_OPCODE_JP_REL8:   // 0x7A,
	case I486_OPCODE_JNP_REL8:  // 0x7B,
	case I486_OPCODE_JL_REL8:   // 0x7C,
	case I486_OPCODE_JGE_REL8:  // 0x7D,
	case I486_OPCODE_JLE_REL8:  // 0x7E,
	case I486_OPCODE_JG_REL8:   // 0x7F,
		{
			bool jumpCond=false;
			switch(inst.opCode)
			{
			case I486_OPCODE_JMP_REL8://         0xEB,   // cb
				jumpCond=true;
				break;
			case I486_OPCODE_JO_REL8:   // 0x70,
				jumpCond=CondJO();
				break;
			case I486_OPCODE_JNO_REL8:  // 0x71,
				jumpCond=CondJNO();
				break;
			case I486_OPCODE_JB_REL8:   // 0x72,
				jumpCond=CondJB();
				break;
			case I486_OPCODE_JAE_REL8:  // 0x73,
				jumpCond=CondJAE();
				break;
			case I486_OPCODE_JE_REL8:   // 0x74,
				jumpCond=CondJE();
				break;
			case I486_OPCODE_JNE_REL8:  // 0x75,
				jumpCond=CondJNE();
				break;
			case I486_OPCODE_JBE_REL8:  // 0x76,
				jumpCond=CondJBE();
				break;
			case I486_OPCODE_JA_REL8:   // 0x77,
				jumpCond=CondJA();
				break;
			case I486_OPCODE_JS_REL8:   // 0x78,
				jumpCond=CondJS();
				break;
			case I486_OPCODE_JNS_REL8:  // 0x79,
				jumpCond=CondJNS();
				break;
			case I486_OPCODE_JP_REL8:   // 0x7A,
				jumpCond=CondJP();
				break;
			case I486_OPCODE_JNP_REL8:  // 0x7B,
				jumpCond=CondJNP();
				break;
			case I486_OPCODE_JL_REL8:   // 0x7C,
				jumpCond=CondJL();
				break;
			case I486_OPCODE_JGE_REL8:  // 0x7D,
				jumpCond=CondJGE();
				break;
			case I486_OPCODE_JLE_REL8:  // 0x7E,
				jumpCond=CondJLE();
				break;
			case I486_OPCODE_JG_REL8:   // 0x7F,
				jumpCond=CondJG();
				break;
			}
			if(true==jumpCond)
			{
				auto offset=inst.GetSimm8();
				auto destin=state.EIP+offset+inst.numBytes;
				if(16==inst.operandSize)
				{
					destin&=0xffff;
				}
				state.EIP=destin;
				clocksPassed=3;
				EIPChanged=true;
			}
			else
			{
				clocksPassed=1;
			}
		}
		break;


	case I486_OPCODE_JMP_FAR:
		{
			switch(inst.operandSize)
			{
			case 16:
				if(true==IsInRealMode())
				{
					clocksPassed=17;
				}
				else
				{
					clocksPassed=19;
				}
				break;
			case 32:
				if(true==IsInRealMode())
				{
					clocksPassed=13;
				}
				else
				{
					clocksPassed=18;
				}
				break;
			}
			LoadSegmentRegister(state.CS(),op1.seg,mem);
			state.EIP=op1.offset;
			EIPChanged=true;
		}
		break;


	case I486_OPCODE_BINARYOP_RM8_FROM_I8://=  0x80, // AND(REG=4), OR(REG=1), or XOR(REG=6) depends on the REG field of MODR/M
		{
			if(op1.operandType==OPER_ADDR || op2.operandType==OPER_ADDR)
			{
				clocksPassed=3;
			}
			else
			{
				clocksPassed=1;
			}

			auto value1=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op1,1);
			auto value2=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op2,1);
			if(true==state.exception)
			{
				break;
			}

			auto i=value1.GetAsDword();
			auto REG=inst.GetREG();
			switch(REG)
			{
			case 0:
				AddByte(i,value2.GetAsDword());
				break;
			case 1:
				OrByte(i,value2.GetAsDword());
				break;
			case 2:
				AdcByte(i,value2.GetAsDword());
				break;
			case 3:
				SbbByte(i,value2.GetAsDword());
				break;
			case 4:
				AndByte(i,value2.GetAsDword());
				break;
			case 5:
				SubByte(i,value2.GetAsDword());
				break;
			case 6:
				XorByte(i,value2.GetAsDword());
				break;
			case 7: // CMP
				SubByte(i,value2.GetAsDword());
				break;
			default:
				Abort("Undefined REG for "+cpputil::Ubtox(inst.opCode));
				break;
			}
			if(7!=REG) // Don't store a value if it is CMP
			{
				value1.SetDword(i);
				StoreOperandValue(op1,mem,inst.addressSize,inst.segOverride,value1);
			}
		}
	case I486_OPCODE_BINARYOP_R_FROM_I://=     0x81,
	case I486_OPCODE_BINARYOP_RM_FROM_SXI8://= 0x83, Sign of op2 is already extended when decoded.
		{
			if(op1.operandType==OPER_ADDR || op2.operandType==OPER_ADDR)
			{
				clocksPassed=3;
			}
			else
			{
				clocksPassed=1;
			}

			auto value1=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op1,inst.operandSize/8);
			auto value2=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op2,inst.operandSize/8);
			if(true==state.exception)
			{
				break;
			}

			auto i=value1.GetAsDword();
			auto REG=inst.GetREG();
			switch(REG)
			{
			case 0:
				AddWordOrDword(inst.operandSize,i,value2.GetAsDword());
				break;
			case 1:
				OrWordOrDword(inst.operandSize,i,value2.GetAsDword());
				break;
			case 2:
				AdcWordOrDword(inst.operandSize,i,value2.GetAsDword());
				break;
			case 3:
				SbbWordOrDword(inst.operandSize,i,value2.GetAsDword());
				break;
			case 4:
				AndWordOrDword(inst.operandSize,i,value2.GetAsDword());
				break;
			case 5:
				SubWordOrDword(inst.operandSize,i,value2.GetAsDword());
				break;
			case 6:
				XorWordOrDword(inst.operandSize,i,value2.GetAsDword());
				break;
			case 7:
				SubWordOrDword(inst.operandSize,i,value2.GetAsDword());
				break;
			default:
				Abort("Undefined REG for "+cpputil::Ubtox(inst.opCode));
				break;
			}
			if(7!=REG) // Don't store a value if it is CMP
			{
				value1.SetDword(i);
				StoreOperandValue(op1,mem,inst.addressSize,inst.segOverride,value1);
			}
		}
		break;


	case I486_OPCODE_LGDT_LIDT:
		switch(inst.GetREG())
		{
		case 2: // LGDT
		case 3: // LIDT
			clocksPassed=11;
			if(OPER_ADDR==op1.operandType)
			{
				auto numBytes=(inst.operandSize+16)/8;
				auto value=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op1,numBytes);
				switch(inst.GetREG())
				{
				case 2:
					LoadDescriptorTableRegister(state.GDTR,inst.operandSize,value.byteData);
					break;
				case 3:
					LoadDescriptorTableRegister(state.IDTR,inst.operandSize,value.byteData);
					break;
				}
			}
			else
			{
				if(IsInRealMode())
				{
					Interrupt(6);
					EIPChanged=true;
				}
				else
				{
					RaiseException(EXCEPTION_UD,0);
					EIPChanged=true;
				}
			}
			break;
		default:
			Abort("Undefined REG for "+cpputil::Ubtox(inst.opCode));
			break;
		}
		break;


	case I486_OPCODE_MOV_FROM_SEG: //     0x8C,
		Move(mem,inst.addressSize,inst.segOverride,op1,op2);
		clocksPassed=3;
		break;
	case I486_OPCODE_MOV_TO_SEG: //       0x8E,
		Move(mem,inst.addressSize,inst.segOverride,op1,op2);
		if(true==IsInRealMode())
		{
			clocksPassed=3;
		}
		else
		{
			clocksPassed=9;
		}
		break;
	case I486_OPCODE_MOV_FROM_R8: //      0x88,
	case I486_OPCODE_MOV_FROM_R: //       0x89, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_TO_R8: //        0x8A,
	case I486_OPCODE_MOV_TO_R: //         0x8B, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_M_TO_AL: //      0xA0,
	case I486_OPCODE_MOV_M_TO_EAX: //     0xA1, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_M_FROM_AL: //    0xA2,
	case I486_OPCODE_MOV_M_FROM_EAX: //   0xA3, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I8_TO_AL: //     0xB0,
	case I486_OPCODE_MOV_I8_TO_CL: //     0xB1,
	case I486_OPCODE_MOV_I8_TO_DL: //     0xB2,
	case I486_OPCODE_MOV_I8_TO_BL: //     0xB3,
	case I486_OPCODE_MOV_I8_TO_AH: //     0xB4,
	case I486_OPCODE_MOV_I8_TO_CH: //     0xB5,
	case I486_OPCODE_MOV_I8_TO_DH: //     0xB6,
	case I486_OPCODE_MOV_I8_TO_BH: //     0xB7,
	case I486_OPCODE_MOV_I_TO_EAX: //     0xB8, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_ECX: //     0xB9, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_EDX: //     0xBA, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_EBX: //     0xBB, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_ESP: //     0xBC, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_EBP: //     0xBD, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_ESI: //     0xBE, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I_TO_EDI: //     0xBF, // 16/32 depends on OPSIZE_OVERRIDE
	case I486_OPCODE_MOV_I8_TO_RM8: //    0xC6,
	case I486_OPCODE_MOV_I_TO_RM: //      0xC7, // 16/32 depends on OPSIZE_OVERRIDE
		Move(mem,inst.addressSize,inst.segOverride,op1,op2);
		clocksPassed=1;
		break;

	case I486_OPCODE_MOV_TO_CR://        0x220F,
		Move(mem,inst.addressSize,inst.segOverride,op1,op2);
		clocksPassed=16;
		break;
	case I486_OPCODE_MOV_FROM_CR://      0x200F,
		Move(mem,inst.addressSize,inst.segOverride,op1,op2);
		clocksPassed=4;
		break;
	case I486_OPCODE_MOV_FROM_DR://      0x210F,
		Move(mem,inst.addressSize,inst.segOverride,op1,op2);
		clocksPassed=10;
		break;
	case I486_OPCODE_MOV_TO_DR://        0x230F,
		Move(mem,inst.addressSize,inst.segOverride,op1,op2);
		clocksPassed=11;
		break;
	case I486_OPCODE_MOV_FROM_TR://      0x240F,
		Move(mem,inst.addressSize,inst.segOverride,op1,op2);
		clocksPassed=4;  // 3 for TR3 strictly speaking.
		break;
	case I486_OPCODE_MOV_TO_TR://        0x260F,
		Move(mem,inst.addressSize,inst.segOverride,op1,op2);
		clocksPassed=4;  // 6 for TR6 strictly speaking.
		break;


	case I486_OPCODE_MOVZX_R_RM8://=      0xB60F, 8bit to 16or32bit
		{
			clocksPassed=3;
			auto value=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op2,1);
			if(true!=state.exception)
			{
				value.numBytes=1;
				value.byteData[1]=0;
				value.byteData[2]=0;
				value.byteData[3]=0;
				StoreOperandValue(op1,mem,inst.addressSize,inst.segOverride,value);
			}
		}
		break;
	case I486_OPCODE_MOVZX_R32_RM16://=   0xB70F, 16bit to 32bit
		{
			clocksPassed=3;
			auto value=EvaluateOperand(mem,inst.addressSize,inst.segOverride,op2,2);
			if(true!=state.exception)
			{
				value.numBytes=2;
				value.byteData[2]=0;
				value.byteData[3]=0;
				StoreOperandValue(op1,mem,inst.addressSize,inst.segOverride,value);
			}
		}
		break;


	case I486_OPCODE_OUT_I8_AL: //        0xE6,
		io.Out8(inst.operand[0],GetRegisterValue(REG_AL));
		if(true==IsInRealMode())
		{
			clocksPassed=16;
		}
		else
		{
			clocksPassed=11; // 31 if CPL>IOPL
		}
		break;
	case I486_OPCODE_OUT_I8_A: //         0xE7,
		if(16==inst.operandSize)
		{
			io.Out16(inst.operand[0],GetRegisterValue(REG_AX));
		}
		else
		{
			io.Out32(inst.operand[0],GetRegisterValue(REG_EAX));
		}
		if(true==IsInRealMode())
		{
			clocksPassed=16;
		}
		else
		{
			clocksPassed=11; // 31 if CPL>IOPL
		}
		break;
	case I486_OPCODE_OUT_DX_AL: //        0xEE,
		io.Out8(GetRegisterValue(REG_DX),GetRegisterValue(REG_AL));
		if(true==IsInRealMode())
		{
			clocksPassed=16;
		}
		else
		{
			clocksPassed=10; // 30 if CPL>IOPL
		}
		break;
	case I486_OPCODE_OUT_DX_A: //         0xEF,
		if(16==inst.operandSize)
		{
			io.Out16(GetRegisterValue(REG_DX),GetRegisterValue(REG_AX));
		}
		else
		{
			io.Out32(GetRegisterValue(REG_DX),GetRegisterValue(REG_EAX));
		}
		if(true==IsInRealMode())
		{
			clocksPassed=16;
		}
		else
		{
			clocksPassed=10; // 30 if CPL>IOPL
		}
		break;


	case I486_OPCODE_PUSHA://            0x60,
		clocksPassed=11;
		{
			auto temp=state.ESP();
			Push(mem,inst.operandSize,state.EAX());
			Push(mem,inst.operandSize,state.ECX());
			Push(mem,inst.operandSize,state.EDX());
			Push(mem,inst.operandSize,state.EBX());
			Push(mem,inst.operandSize,temp);
			Push(mem,inst.operandSize,state.EBP());
			Push(mem,inst.operandSize,state.ESI());
			Push(mem,inst.operandSize,state.EDI());
		}
		break;
	case I486_OPCODE_PUSHF://            0x9C,
		clocksPassed=4; // If running as 386 and in protected mode, 3 clocks.
		{
			Push(mem,inst.operandSize,state.EFLAGS);
		}
		break;


	case I486_OPCODE_PUSH_EAX://         0x50,
	case I486_OPCODE_PUSH_ECX://         0x51,
	case I486_OPCODE_PUSH_EDX://         0x52,
	case I486_OPCODE_PUSH_EBX://         0x53,
	case I486_OPCODE_PUSH_ESP://         0x54,
	case I486_OPCODE_PUSH_EBP://         0x55,
	case I486_OPCODE_PUSH_ESI://         0x56,
	case I486_OPCODE_PUSH_EDI://         0x57,
		Push(mem,inst.operandSize,state.reg32[(inst.opCode&7)]);
		break;
	case I486_OPCODE_PUSH_I8://          0x6A,
		Push(mem,inst.operandSize,inst.GetUimm8());
		break;
	case I486_OPCODE_PUSH_I://           0x68,
		Push(mem,inst.operandSize,inst.GetUimm32());
		break;
	case I486_OPCODE_PUSH_CS://          0x0E,
		Push(mem,inst.operandSize,state.CS().value);
		clocksPassed=3;
		break;
	case I486_OPCODE_PUSH_SS://          0x16,
		Push(mem,inst.operandSize,state.SS().value);
		clocksPassed=3;
		break;
	case I486_OPCODE_PUSH_DS://          0x1E,
		Push(mem,inst.operandSize,state.DS().value);
		clocksPassed=3;
		break;
	case I486_OPCODE_PUSH_ES://          0x06,
		Push(mem,inst.operandSize,state.ES().value);
		clocksPassed=3;
		break;
	case I486_OPCODE_PUSH_FS://          0xA00F,
		Push(mem,inst.operandSize,state.FS().value);
		clocksPassed=3;
		break;
	case I486_OPCODE_PUSH_GS://          0xA80F,
		Push(mem,inst.operandSize,state.GS().value);
		clocksPassed=3;
		break;


	case I486_OPCODE_POP_M://            0x8F,
		break;
	case I486_OPCODE_POP_EAX://          0x58,
	case I486_OPCODE_POP_ECX://          0x59,
	case I486_OPCODE_POP_EDX://          0x5A,
	case I486_OPCODE_POP_EBX://          0x5B,
	case I486_OPCODE_POP_ESP://          0x5C,
	case I486_OPCODE_POP_EBP://          0x5D,
	case I486_OPCODE_POP_ESI://          0x5E,
	case I486_OPCODE_POP_EDI://          0x5F,
		break;
	case I486_OPCODE_POP_SS://           0x17,
		break;
	case I486_OPCODE_POP_DS://           0x1F,
		break;
	case I486_OPCODE_POP_ES://           0x07,
		break;
	case I486_OPCODE_POP_FS://           0xA10F,
		break;
	case I486_OPCODE_POP_GS://           0xA90F,
		break;

	case I486_OPCODE_POPA://             0x61,
		break;

	case I486_OPCODE_POPF://             0x9D,
		SetFLAGSorEFLAGS(inst.operandSize,Pop(mem,inst.operandSize));
		clocksPassed=(IsInRealMode() ? 9 : 6);
		break;


	case I486_OPCODE_RET://              0xC3,
		clocksPassed=5;
		SetIPorEIP(inst.operandSize,Pop(mem,inst.operandSize));
		EIPChanged=true;
		break;
	case I486_OPCODE_RETF://             0xCB,
		if(true==IsInRealMode())
		{
			clocksPassed=13;
		}
		else
		{
			clocksPassed=18;
		}
		SetIPorEIP(inst.operandSize,Pop(mem,inst.operandSize));
		LoadSegmentRegister(state.CS(),Pop(mem,inst.operandSize),mem);
		state.ESP()+=inst.GetUimm16(); // Do I need to take &0xffff if address mode is 16? 
		EIPChanged=true;
		break;
	case I486_OPCODE_RET_I16://          0xC2,
		clocksPassed=5;
		SetIPorEIP(inst.operandSize,Pop(mem,inst.operandSize));
		EIPChanged=true;
		break;
	case I486_OPCODE_RETF_I16://         0xCA,
		if(true==IsInRealMode())
		{
			clocksPassed=14;
		}
		else
		{
			clocksPassed=17;
		}
		SetIPorEIP(inst.operandSize,Pop(mem,inst.operandSize));
		LoadSegmentRegister(state.CS(),Pop(mem,inst.operandSize),mem);
		state.ESP()+=inst.GetUimm16(); // Do I need to take &0xffff if address mode is 16? 
		EIPChanged=true;
		break;


	default:
		Abort("Undefined instruction or simply not supported yet.");
		break;
	}

	if(true!=abort && 0==clocksPassed)
	{
		Abort("Clocks-Passed is not set.");
	}
	if(true!=EIPChanged && true!=abort)
	{
		state.EIP+=inst.numBytes;
	}

	return clocksPassed;
}
