#ifndef TOWNSPARAM_IS_INCLUDED
#define TOWNSPARAM_IS_INCLUDED
/* { */



#include <string>
#include <vector>
#include "townsdef.h"

class TownsStartParameters
{
public:
	class VirtualKey
	{
	public:
		std::string townsKey;
		int physicalId;
		unsigned int button;
	};

	unsigned int townsType=TOWNSTYPE_UNKNOWN;

	unsigned int gamePort[2]={TOWNS_GAMEPORTEMU_PHYSICAL0,TOWNS_GAMEPORTEMU_MOUSE};

	unsigned int bootKeyComb=BOOT_KEYCOMB_NONE;

	bool windowShift=false;

	bool noWait=true;
	bool noWaitStandby=false;

	bool highResAvailable=true;

	bool catchUpRealTime=true;

	bool damperWireLine=false;
	bool scanLineEffectIn15KHz=false;

	std::string ROMPath;
	std::string CMOSFName;
	bool autoSaveCMOS=true; // If this flag is false, CMOSFName will not be copied to FMTowns::Variable.
	std::string fdImgFName[2];
	bool fdImgWriteProtect[2]={false,false};
	std::string cdImgFName;
	unsigned int memCardType=TOWNS_MEMCARD_TYPE_NONE;
	std::string memCardImgFName;
	std::string startUpScriptFName;
	std::string symbolFName;
	std::string playbackEventLogFName;
	std::string keyMapFName;

	std::vector <std::string> sharedDir;

	std::vector <std::string> fdSearchPaths;
	std::vector <std::string> cdSearchPaths;

	int keyboardMode=TOWNS_KEYBOARD_MODE_DEFAULT;
	std::vector <VirtualKey> virtualKeys;

	int fmVol=-1,pcmVol=-1;

	bool mouseByFlightstickAvailable=false;
	bool cyberStickAssignment=false;
	int mouseByFlightstickPhysicalId=-1;  // Physical joystick ID.
	int mouseByFlightstickCenterX=320,mouseByFlightstickCenterY=200;
	float mouseByFlightstickZeroZoneX=0.0F,mouseByFlightstickZeroZoneY=0.0F;
	float mouseByFlightstickScaleX=500.0F,mouseByFlightstickScaleY=400.0F;


	// When adding a throttle control for an app, also update Outside_World::CacheGamePadIndicesThatNeedUpdates.
	int throttlePhysicalId=-1;
	int throttleAxis=3;

	enum
	{
		SCSIIMAGE_HARDDISK,
		SCSIIMAGE_CDROM
	};
	class SCSIImage
	{
	public:
		unsigned int scsiID;
		unsigned imageType;
		std::string imgFName;
	};
	std::vector <SCSIImage> scsiImg;

	class FileToSend
	{
	public:
		std::string hostFName,vmFName;
	};
	std::vector <FileToSend> toSend;

	bool autoStart=true;
	bool debugger=false;
	bool interactive=true;

	bool powerOffAtBreakPoint=false;
	bool forceQuitOnPowerOff=false;
	std::string powerOffAt;

	bool pretend386DX=false;

	unsigned int memSizeInMB=4;

	unsigned int freq=33;

	unsigned int mouseIntegrationSpeed=256;

	unsigned int appSpecificSetting=0;

	enum
	{
		SCALING_DEFAULT=100,
		SCALING_MIN=50,
		SCALING_MAX=1000,
	};
	unsigned int scaling=100;
	bool autoScaling=false;
	bool maximizeOnStartUp=false;

	TownsStartParameters();
};


/* } */
#endif