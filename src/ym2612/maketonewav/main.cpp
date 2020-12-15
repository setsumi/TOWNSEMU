// Experimental program to visualize a wave from YM2612.
// The purpose is to explore the correct way to calculate feedback.

#include "fssimplewindow.h"
#include "ym2612.h"
#include "cpputil.h"
#include <yssimplesound.h>



unsigned char sample[]=
{
	0x00,0x30,0x01,//(1)
	0x00,0x34,0x01,//(1)
	0x00,0x38,0x01,//(1)
	0x00,0x3C,0x01,//(1)
	0x00,0x40,0x7F,//(1)
	0x00,0x44,0x7F,//(1)
	0x00,0x48,0x7F,//(1)
	0x00,0x4C,0x7F,//(1)
	0x00,0x50,0x1F,//(1)
	0x00,0x54,0x1F,//(1)
	0x00,0x58,0x1F,//(1)
	0x00,0x5C,0x1F,//(1)
	0x00,0x60,0x00,//(1)
	0x00,0x64,0x00,//(1)
	0x00,0x68,0x00,//(1)
	0x00,0x6C,0x00,//(1)
	0x00,0x70,0x00,//(1)
	0x00,0x74,0x00,//(1)
	0x00,0x78,0x00,//(1)
	0x00,0x7C,0x00,//(1)
	0x00,0x80,0x0F,//(1)
	0x00,0x84,0x0F,//(1)
	0x00,0x88,0x0F,//(1)
	0x00,0x8C,0x0F,//(1)
	0x00,0xB0,0x17,//(1)
	0x00,0xB4,0xC0,//(1)
	0x00,0x31,0x01,//(1)
	0x00,0x35,0x01,//(1)
	0x00,0x39,0x01,//(1)
	0x00,0x3D,0x01,//(1)
	0x00,0x41,0x7F,//(1)
	0x00,0x45,0x7F,//(1)
	0x00,0x49,0x7F,//(1)
	0x00,0x4D,0x7F,//(1)
	0x00,0x51,0x1F,//(1)
	0x00,0x55,0x1F,//(1)
	0x00,0x59,0x1F,//(1)
	0x00,0x5D,0x1F,//(1)
	0x00,0x61,0x00,//(1)
	0x00,0x65,0x00,//(1)
	0x00,0x69,0x00,//(1)
	0x00,0x6D,0x00,//(1)
	0x00,0x71,0x00,//(1)
	0x00,0x75,0x00,//(1)
	0x00,0x79,0x00,//(1)
	0x00,0x7D,0x00,//(1)
	0x00,0x81,0x0F,//(1)
	0x00,0x85,0x0F,//(1)
	0x00,0x89,0x0F,//(1)
	0x00,0x8D,0x0F,//(1)
	0x00,0xB1,0x17,//(1)
	0x00,0xB5,0xC0,//(1)
	0x00,0x32,0x01,//(1)
	0x00,0x36,0x01,//(1)
	0x00,0x3A,0x01,//(1)
	0x00,0x3E,0x01,//(1)
	0x00,0x42,0x7F,//(1)
	0x00,0x46,0x7F,//(1)
	0x00,0x4A,0x7F,//(1)
	0x00,0x4E,0x7F,//(1)
	0x00,0x52,0x1F,//(1)
	0x00,0x56,0x1F,//(1)
	0x00,0x5A,0x1F,//(1)
	0x00,0x5E,0x1F,//(1)
	0x00,0x62,0x00,//(1)
	0x00,0x66,0x00,//(1)
	0x00,0x6A,0x00,//(1)
	0x00,0x6E,0x00,//(1)
	0x00,0x72,0x00,//(1)
	0x00,0x76,0x00,//(1)
	0x00,0x7A,0x00,//(1)
	0x00,0x7E,0x00,//(1)
	0x00,0x82,0x0F,//(1)
	0x00,0x86,0x0F,//(1)
	0x00,0x8A,0x0F,//(1)
	0x00,0x8E,0x0F,//(1)
	0x00,0xB2,0x17,//(1)
	0x00,0xB6,0xC0,//(1)
	0x03,0x30,0x01,//(1)
	0x03,0x34,0x01,//(1)
	0x03,0x38,0x01,//(1)
	0x03,0x3C,0x01,//(1)
	0x03,0x40,0x7F,//(1)
	0x03,0x44,0x7F,//(1)
	0x03,0x48,0x7F,//(1)
	0x03,0x4C,0x7F,//(1)
	0x03,0x50,0x1F,//(1)
	0x03,0x54,0x1F,//(1)
	0x03,0x58,0x1F,//(1)
	0x03,0x5C,0x1F,//(1)
	0x03,0x60,0x00,//(1)
	0x03,0x64,0x00,//(1)
	0x03,0x68,0x00,//(1)
	0x03,0x6C,0x00,//(1)
	0x03,0x70,0x00,//(1)
	0x03,0x74,0x00,//(1)
	0x03,0x78,0x00,//(1)
	0x03,0x7C,0x00,//(1)
	0x03,0x80,0x0F,//(1)
	0x03,0x84,0x0F,//(1)
	0x03,0x88,0x0F,//(1)
	0x03,0x8C,0x0F,//(1)
	0x03,0xB0,0x17,//(1)
	0x03,0xB4,0xC0,//(1)
	0x03,0x31,0x01,//(1)
	0x03,0x35,0x01,//(1)
	0x03,0x39,0x01,//(1)
	0x03,0x3D,0x01,//(1)
	0x03,0x41,0x7F,//(1)
	0x03,0x45,0x7F,//(1)
	0x03,0x49,0x7F,//(1)
	0x03,0x4D,0x7F,//(1)
	0x03,0x51,0x1F,//(1)
	0x03,0x55,0x1F,//(1)
	0x03,0x59,0x1F,//(1)
	0x03,0x5D,0x1F,//(1)
	0x03,0x61,0x00,//(1)
	0x03,0x65,0x00,//(1)
	0x03,0x69,0x00,//(1)
	0x03,0x6D,0x00,//(1)
	0x03,0x71,0x00,//(1)
	0x03,0x75,0x00,//(1)
	0x03,0x79,0x00,//(1)
	0x03,0x7D,0x00,//(1)
	0x03,0x81,0x0F,//(1)
	0x03,0x85,0x0F,//(1)
	0x03,0x89,0x0F,//(1)
	0x03,0x8D,0x0F,//(1)
	0x03,0xB1,0x17,//(1)
	0x03,0xB5,0xC0,//(1)
	0x03,0x32,0x01,//(1)
	0x03,0x36,0x01,//(1)
	0x03,0x3A,0x01,//(1)
	0x03,0x3E,0x01,//(1)
	0x03,0x42,0x7F,//(1)
	0x03,0x46,0x7F,//(1)
	0x03,0x4A,0x7F,//(1)
	0x03,0x4E,0x7F,//(1)
	0x03,0x52,0x1F,//(1)
	0x03,0x56,0x1F,//(1)
	0x03,0x5A,0x1F,//(1)
	0x03,0x5E,0x1F,//(1)
	0x03,0x62,0x00,//(1)
	0x03,0x66,0x00,//(1)
	0x03,0x6A,0x00,//(1)
	0x03,0x6E,0x00,//(1)
	0x03,0x72,0x00,//(1)
	0x03,0x76,0x00,//(1)
	0x03,0x7A,0x00,//(1)
	0x03,0x7E,0x00,//(1)
	0x03,0x82,0x0F,//(1)
	0x03,0x86,0x0F,//(1)
	0x03,0x8A,0x0F,//(1)
	0x03,0x8E,0x0F,//(1)
	0x03,0xB2,0x17,//(1)
	0x03,0xB6,0xC0,//(1)

	0x00,0xA5,0x22,//(1)
	0x00,0xA1,0x6B,//(1)
	0x00,0x41,0x08,//(1)
	0x00,0x45,0x7E,//(1)
	0x00,0x49,0x7E,//(1)
	0x00,0x4D,0x7E,//(1)
	0x00,0x81,0x0F,//(1)
	0x00,0x85,0x0F,//(1)
	0x00,0x89,0x0F,//(1)
	0x00,0x8D,0x0F,//(1)
	0x00,0x28,0x01,//(1)
	0x00,0x81,0x0F,//(1)
	0x00,0x85,0x0F,//(1)
	0x00,0x89,0x0F,//(1)
	0x00,0x8D,0x0F,//(1)
};

void SetUpYM(YM2612 &ym2612,int CONNECT,int FB,int MULTI)
{
	for(int i=0; i<sizeof(sample)/sizeof(sample[0]); i+=3)
	{
		if(0x31==sample[i+1])
		{
			ym2612.WriteRegister(sample[i],sample[i+1],(sample[i+2]&0xF0)|MULTI);
		}
		else
		{
			ym2612.WriteRegister(sample[i],sample[i+1],sample[i+2]);
		}
	}

	ym2612.WriteRegister(0,0xB1,(FB<<3)|CONNECT);  // Ch1 not CH0
}

void StartTone(YM2612 &ym2612)
{
	ym2612.WriteRegister(0,0x28,0xF0); // Start Tone CH1
}

void StopTone(YM2612 &ym2612)
{
	ym2612.WriteRegister(0,0x28,0x00); // Stop Tone CH1
}

std::vector <unsigned char> MakeSample(YM2612 &ym2612)
{
	ym2612.KeyOn(0);
	auto wave=ym2612.MakeWave(0,1000); // 1sec
	ym2612.KeyOff(0);
	return wave;
}

int main(void)
{
	// FB=5,6,7 depends on the sampling cycle.
	//     440Hz -> cycle=0.002272727272... second
	//     If YM2612 internal frequency is 600KHz as explained in FM Towns Technical Databook,
	//     (1/600K)/0.02272727272=(1/600K)*440=440/600K=0.000733 seconds.
	//     If it is 690KHz as my obbservation,
	//     440/690K=0.000638 seconds.
	// 1.5K?  No way!  Am I calculating wrong?

	// PrintTable();

	YM2612 ym2612;

	// CH:3  F_NUM=  692  BLOCK=3  FB=7  CONNECT=5  L=1  R=1  AMS=0  PMS=0  ActiveSlots=00

	ym2612.state.LFO=false;

	ym2612.state.channels[0].F_NUM=692;
	ym2612.state.channels[0].BLOCK=3;
	ym2612.state.channels[0].FB=7;
	ym2612.state.channels[0].CONNECT=5;
	ym2612.state.channels[0].L=1;
	ym2612.state.channels[0].R=1;
	ym2612.state.channels[0].AMS=0;
	ym2612.state.channels[0].PMS=0;
	ym2612.state.channels[0].usingSlot=0x0F;

	// SLOT:DT=0  MULTI= 1  TL= 26(19dB)  KS=0  AR=15  AM=0  DR=10  SR= 0  SL= 2( 6dB)  RR= 5  SSG_EG=0
	ym2612.state.channels[0].slots[0].DT=0;
	ym2612.state.channels[0].slots[0].MULTI=1;
	ym2612.state.channels[0].slots[0].TL=26;
	ym2612.state.channels[0].slots[0].KS=0;
	ym2612.state.channels[0].slots[0].AR=15;
	ym2612.state.channels[0].slots[0].AM=0;
	ym2612.state.channels[0].slots[0].DR=10;
	ym2612.state.channels[0].slots[0].SR= 0;
	ym2612.state.channels[0].slots[0].SL= 2;
	ym2612.state.channels[0].slots[0].RR= 5;
	ym2612.state.channels[0].slots[0].SSG_EG=0;

	// SLOT:DT=0  MULTI= 1  TL= 13( 9dB)  KS=0  AR=21  AM=0  DR=31  SR= 0  SL= 0( 0dB)  RR=10  SSG_EG=0
	// SLOT:DT=0  MULTI= 1  TL= 13( 9dB)  KS=0  AR=21  AM=0  DR=31  SR= 0  SL= 0( 0dB)  RR=10  SSG_EG=0
	// SLOT:DT=0  MULTI= 1  TL= 13( 9dB)  KS=0  AR=21  AM=0  DR=31  SR= 0  SL= 0( 0dB)  RR=10  SSG_EG=0
	for(int s=1; s<4; ++s)
	{
		ym2612.state.channels[0].slots[s].DT=0;
		ym2612.state.channels[0].slots[s].MULTI= 1;
		ym2612.state.channels[0].slots[s].TL= 13;
		ym2612.state.channels[0].slots[s].KS=0;
		ym2612.state.channels[0].slots[s].AR=21;
		ym2612.state.channels[0].slots[s].AM=0;
		ym2612.state.channels[0].slots[s].DR=31;
		ym2612.state.channels[0].slots[s].SR= 0;
		ym2612.state.channels[0].slots[s].SL= 0;
		ym2612.state.channels[0].slots[s].RR=10;
		ym2612.state.channels[0].slots[s].SSG_EG=0;
	}

	std::vector <unsigned char> wave=MakeSample(ym2612);

	YsSoundPlayer::SoundData data;
	data.CreateFromSigned16bitStereo(44100,wave);
	auto wavFile=data.MakeWavByteData();
	cpputil::WriteBinaryFile("wave.wav",wavFile.size(),wavFile.data());

	return 0;
}
