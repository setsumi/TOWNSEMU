#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "io.h"

#include "snd.h"




char SND_work[SndWorkSize];



struct FMInst
{
	unsigned char DT_MULTI[4];
	unsigned char TL[4];
	unsigned char KS_AR[4];
	unsigned char AMON_DR[4];
	unsigned char SR[4];
	unsigned char SL_RR[4];
	unsigned char FB_CONNECT;
	unsigned char LR_AMS_PMS;
	unsigned char MAKE_IT_16_TIMES_3[14];
};

struct FMB
{
	char bankName[8];
	struct FMInst inst[128];
};

void ClearFMB(struct FMB *fmbPtr)
{
	unsigned char *ptr=(unsigned char *)fmbPtr;
	for(int i=0; i<sizeof(struct FMB); ++i)
	{
		ptr[i]=0;
	}
}


#define MIX_DT_MULTI(DT,MULTI) ((((DT)&7)<<4)|(MULTI&15))
#define MIX_KS_AR(KS,AR) (((KS&3)<<6)|(AR&31))
#define MIX_AMON_DR(AMON,DR) (((AMON&1)<<7)|(DR&31))
#define MIX_SL_RR(SL,RR) (((SL&15)<<4)|(RR&15))
#define MIX_FB_CONNECT(FB,CONNECT) (((FB&7)<<3)|(CONNECT&7))
#define MIX_LR_AMS_PMS(L,R,AMS,PMS) (((L&1)<<7)|((R&1)<<6)|((AMS&3)<<4)|(PMS&7))

static struct FMB fmb;


#define IO_YM2612_STATUS		0x4D8
#define IO_YM2612_ADDR_CH0_2	0x4D8
#define IO_YM2612_DATA_CH0_2	0x4DA
#define IO_YM2612_ADDR_CH3_5	0x4DC
#define IO_YM2612_DATA_CH3_5	0x4DE

void WriteYM2612RegisterCH0to2(unsigned char reg,unsigned char data)
{
	while(0!=(0x80&IOReadByte(IO_YM2612_STATUS)))
	{
	}
	IOWriteByte(IO_YM2612_ADDR_CH0_2,reg);
	while(0!=(0x80&IOReadByte(IO_YM2612_STATUS)))
	{
	}
	IOWriteByte(IO_YM2612_DATA_CH0_2,data);
}


int main(void)
{
	// 000C:0048


	int F_NUM=692;
	int BLOCK=3;

	ClearFMB(&fmb);

	fmb.inst[0].DT_MULTI[0]=MIX_DT_MULTI(0,1);
	fmb.inst[0].TL      [0]=26;
	fmb.inst[0].KS_AR   [0]=MIX_KS_AR(0,15);
	fmb.inst[0].AMON_DR [0]=MIX_AMON_DR(0,10);
	fmb.inst[0].SR      [0]=0;
	fmb.inst[0].SL_RR   [0]=MIX_SL_RR(2,5);

	fmb.inst[0].DT_MULTI[1]=MIX_DT_MULTI(0,1);
	fmb.inst[0].TL      [1]=13;
	fmb.inst[0].KS_AR   [1]=MIX_KS_AR(0,21);
	fmb.inst[0].AMON_DR [1]=MIX_AMON_DR(0,31);
	fmb.inst[0].SR      [1]=0;
	fmb.inst[0].SL_RR   [1]=MIX_SL_RR(0,10);

	fmb.inst[0].DT_MULTI[2]=MIX_DT_MULTI(0,1);
	fmb.inst[0].TL      [2]=13;
	fmb.inst[0].KS_AR   [2]=MIX_KS_AR(0,21);
	fmb.inst[0].AMON_DR [2]=MIX_AMON_DR(0,31);
	fmb.inst[0].SR      [2]=0;
	fmb.inst[0].SL_RR   [2]=MIX_SL_RR(0,10);

	fmb.inst[0].DT_MULTI[3]=MIX_DT_MULTI(0,1);
	fmb.inst[0].TL      [3]=13;
	fmb.inst[0].KS_AR   [3]=MIX_KS_AR(0,21);
	fmb.inst[0].AMON_DR [3]=MIX_AMON_DR(0,31);
	fmb.inst[0].SR      [3]=0;
	fmb.inst[0].SL_RR   [3]=MIX_SL_RR(0,10);

	fmb.inst[0].FB_CONNECT=MIX_FB_CONNECT(7,5);
	fmb.inst[0].LR_AMS_PMS=MIX_LR_AMS_PMS(1,1,0,0);


	// Sound BIOS refuses to write to TL for slots 1,2, and 3.
	SND_init(SND_work);
	// SND_inst_write(0,0,(char *)&fmb);
	// SND_inst_change(0,0);

	int instNum=0,ch=0;
	for(int slot=0; slot<4; ++slot)
	{
		WriteYM2612RegisterCH0to2(0x30+ch+slot*4,fmb.inst[instNum].DT_MULTI[slot]);
		WriteYM2612RegisterCH0to2(0x40+ch+slot*4,fmb.inst[instNum].TL[slot]);
		WriteYM2612RegisterCH0to2(0x50+ch+slot*4,fmb.inst[instNum].KS_AR[slot]);
		WriteYM2612RegisterCH0to2(0x60+ch+slot*4,fmb.inst[instNum].AMON_DR[slot]);
		WriteYM2612RegisterCH0to2(0x70+ch+slot*4,fmb.inst[instNum].SR[slot]);
		WriteYM2612RegisterCH0to2(0x80+ch+slot*4,fmb.inst[instNum].SL_RR[slot]);
	}
	WriteYM2612RegisterCH0to2(0xB0+ch,fmb.inst[instNum].FB_CONNECT);
	WriteYM2612RegisterCH0to2(0xB4+ch,fmb.inst[instNum].LR_AMS_PMS);



	for(int repeat=0; repeat<32; ++repeat)
	{
		BLOCK^=1;

		WriteYM2612RegisterCH0to2(0xA0+ch,F_NUM&255);
		WriteYM2612RegisterCH0to2(0xA4+ch,(BLOCK<<3)|((F_NUM>>8)&7));

		WriteYM2612RegisterCH0to2(0x28,0xf0|(ch&7));   // Key On

		clock_t clk0=clock();
		while(clock()<clk0+CLOCKS_PER_SEC/8);

		WriteYM2612RegisterCH0to2(0x28,(ch&7));  // Key Off
	}

	SND_end();

	return 0;
}
