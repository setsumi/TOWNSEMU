#include <iostream>

#include "cpputil.h"
#include "towns.h"
#include "townsdef.h"



void FMTowns::State::PowerOn(void)
{
	Reset();
	townsTime=0;
	freq=FREQUENCY_DEFAULT;
	resetReason=0;
}
void FMTowns::State::Reset(void)
{
	clockBalance=0;
	// resetReason should survive Reset.
}


////////////////////////////////////////////////////////////


FMTowns::Variable::Variable()
{
	freeRunTimerShift=0;
	Reset();
}

void FMTowns::Variable::Reset(void)
{
	// freeRunTimerShift should survive Reset.
}


////////////////////////////////////////////////////////////


FMTowns::FMTowns() : crtc(this)
{
	townsType=TOWNSTYPE_2_MX;

	abort=false;
	allDevices.push_back(&ioRAM);
	allDevices.push_back(&physMem);
	allDevices.push_back(&crtc);

	physMem.SetMainRAMSize(4*1024*1024);

	physMem.SetVRAMSize(1024*1024);
	physMem.SetSpriteRAMSize(512*1024);
	physMem.SetWaveRAMSize(64*1024);

	io.AddDevice(&ioRAM,0x3000,0x3FFF);

	mainRAMAccess.SetPhysicalMemoryPointer(&physMem);
	mem.AddAccess(&mainRAMAccess,0x00000000,0x000BFFFF);
	mem.AddAccess(&mainRAMAccess,0x000F0000,0x000F7FFF);

	mainRAMorFMRVRAMAccess.SetPhysicalMemoryPointer(&physMem);
	mem.AddAccess(&mainRAMorFMRVRAMAccess,0x000C0000,0x000CFFFF);

	dicROMandDicRAMAccess.SetPhysicalMemoryPointer(&physMem);
	mem.AddAccess(&dicROMandDicRAMAccess,0x000D0000,0x000EFFFF);
	mem.AddAccess(&dicROMandDicRAMAccess,0xC2140000,0xC2141FFF);

	mainRAMorSysROMAccess.SetPhysicalMemoryPointer(&physMem);
	mem.AddAccess(&mainRAMorSysROMAccess,0x000F8000,0x000FFFFF);

	if(0x00100000<physMem.state.RAM.size())
	{
		mem.AddAccess(&mainRAMAccess,0x00100000,(unsigned int)physMem.state.RAM.size()-1);
	}

	VRAMAccess.SetPhysicalMemoryPointer(&physMem);
	mem.AddAccess(&VRAMAccess,0x80000000,0x8007FFFF);
	mem.AddAccess(&VRAMAccess,0x80100000,0x8017FFFF);
	mem.AddAccess(&VRAMAccess,0x82000000,0x83FFFFFF); // For IIMX High Resolution Access.

	spriteRAMAccess.SetPhysicalMemoryPointer(&physMem);
	mem.AddAccess(&spriteRAMAccess,0x81000000,0x8101FFFF);

	osROMAccess.SetPhysicalMemoryPointer(&physMem);
	mem.AddAccess(&osROMAccess,0xC2000000,0xC208FFFF);

	waveRAMAccess.SetPhysicalMemoryPointer(&physMem);
	mem.AddAccess(&waveRAMAccess,0xC2200000,0xC2200FFF);

	sysROMAccess.SetPhysicalMemoryPointer(&physMem);
	mem.AddAccess(&sysROMAccess,0xFFFC0000,0xFFFFFFFF);

	// Free-run counter since FM TOWNS 2UG [2] pp.801
	// Didn't it exist since the first model FM TOWNS 2?
	// I vaguely rember I used something similar when I wrote my first flight simulator 
	// submitted to Japan National High School Students' Programming Contest.
	// FM TOWNS 2UG didn't exist then.
	// I'm positive that I was using the second-generation FM TOWNS then.
	// I'll check if I can find the source code from my old backups.

	// Do range I/O mapping first, then do single I/O mapping.
	// Range I/O mapping may wipe single I/O mapping.
	io.AddDevice(this,TOWNSIO_FREERUN_TIMER_LOW/*0x26*/,TOWNSIO_MACHINE_ID_HIGH/*0x31*/);
	io.AddDevice(&crtc,TOWNSIO_MX_HIRES/*0x470*/,TOWNSIO_MX_IMGOUT_ADDR_D3/*0x477*/);
	io.AddDevice(&keyboard,TOWNSIO_KEYBOARD_DATA/*0x600*/,TOWNSIO_KEYBOARD_IRQ/*0x604*/);

	io.AddDevice(this,TOWNSIO_RESET_REASON);
	io.AddDevice(&physMem,TOWNSIO_FMR_VRAM_OR_MAINRAM);

	PowerOn();
}

bool FMTowns::CheckAbort(void) const
{
	bool ab=false;
	if(true==abort)
	{
		std::cout << "FMTowns:" << abortReason << std::endl;
		ab=true;
	}
	if(true==cpu.abort)
	{
		std::cout << cpu.DeviceName() << ':' << cpu.abortReason << std::endl;
		ab=true;
	}
	if(true==physMem.abort)
	{
		std::cout << physMem.DeviceName() << ':' <<  physMem.abortReason << std::endl;
		ab=true;
	}
	for(auto devPtr : allDevices)
	{
		if(true==devPtr->abort)
		{
			std::cout << devPtr->DeviceName() << ':' <<  devPtr->abortReason << std::endl;
			ab=true;
		}
	}
	return ab;
}

bool FMTowns::LoadROMImages(const char dirName[])
{
	if(true!=physMem.LoadROMImages(dirName))
	{
		abort=true;
		abortReason="Unable to load ROM images.";
		return false;
	}
	return true;
}

void FMTowns::PowerOn(void)
{
	state.PowerOn();
	cpu.PowerOn();
	for(auto devPtr : allDevices)
	{
		devPtr->PowerOn();
	}
}
void FMTowns::Reset(void)
{
	var.Reset();
	state.Reset();
	cpu.Reset();
	for(auto devPtr : allDevices)
	{
		devPtr->Reset();
	}
}

unsigned int FMTowns::RunOneInstruction(void)
{
	auto clocksPassed=cpu.RunOneInstruction(mem,io);
	state.clockBalance+=clocksPassed;
	if(state.freq<=state.clockBalance)
	{
		state.townsTime+=(state.clockBalance/state.freq);
		state.clockBalance%=state.freq;
	}
	return clocksPassed;
}


////////////////////////////////////////////////////////////


unsigned int FMTowns::FetchByteCS_EIP(int offset) const
{
	return cpu.FetchInstructionByte(offset,mem);
}

i486DX::Instruction FMTowns::FetchInstruction(void) const
{
	return cpu.FetchInstruction(mem);
}
std::vector <std::string> FMTowns::GetStackText(unsigned int numBytes) const
{
	std::vector <std::string> text;
	for(unsigned int offsetHigh=0; offsetHigh<numBytes; offsetHigh+=16)
	{
		std::string line;
		line="SS+"+cpputil::Uitox(offsetHigh)+":";
		for(unsigned int offsetLow=0; offsetLow<16 && offsetHigh+offsetLow<numBytes; ++offsetLow)
		{
			line+=cpputil::Ubtox(cpu.FetchByte(cpu.state.SS(),cpu.state.ESP()+offsetHigh+offsetLow,mem));
			line.push_back(' ');
		}
		text.push_back(line);
	}
	return text;
}
void FMTowns::PrintStack(unsigned int numBytes) const
{
	for(auto s : GetStackText(numBytes))
	{
		std::cout << s << std::endl;
	}
}
