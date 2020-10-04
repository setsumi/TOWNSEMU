#include <stdio.h>
#include <string>
#include <vector>
#include "iostream"
#include "fstream"
#include "discimg.h"
#include "cpputil.h"



int main(int ac,char *av[])
{
	if(3!=ac)
	{
		fprintf(stderr,"PADBINCUE by CaptainYS\n");
		fprintf(stderr,"Insert padding between the 2048 bytes/sector data track and the audio track.\n");
		fprintf(stderr,"There is no official BIN/CUE data format.  Looks like the CD writer allows\n");
		fprintf(stderr,"the data track to have 2048 bytes/sector, the rest audio track is shifted.\n");
		fprintf(stderr,"This program pads some bytes so that the writer will correctly place audio\n");
		fprintf(stderr,"tracks.\n");
		fprintf(stderr,"\n");
		fprintf(stderr,"This program is based on my guess.  Again, there is no official standard\n");
		fprintf(stderr,"of BIN/CUE data format :-P\n");
		fprintf(stderr,"\n");
		fprintf(stderr,"The BIN/CUE image created by this program won't be correctly recognized\n");
		fprintf(stderr,"by TSUGARU.  Hope your image writer recognizes it correctly.\n");
		fprintf(stderr,"\n");
		fprintf(stderr,"Usage:\n");
		fprintf(stderr,"  padbincue input.CUE output.CUE\n");
		return 1;
	}

	DiscImage input;
	if(DiscImage::ERROR_NOERROR!=input.OpenCUE(av[1]))
	{
		std::cout << "Cannot open .CUE file." << std::endl;
		return 1;
	}

	bool first=true;
	for(auto &trk : input.GetTracks())
	{
		if(true==first && DiscImage::TRACK_MODE1_DATA!=trk.trackType)
		{
			std::cout << "First track is not a MODE1 data track." << std::endl;
			return 1;
		}
		else if(true!=first && DiscImage::TRACK_AUDIO!=trk.trackType)
		{
			std::cout << "Non-Audio track in 2nd or later track." << std::endl;
			return 1;
		}
		first=false;
	}

	if(true==first)
	{
		std::cout << "No track." << std::endl;
		return 1;
	}
	if(1==input.GetTracks().size())
	{
		std::cout << "No audio track." << std::endl;
		return 1;
	}
	if(2048!=input.GetTracks()[0].sectorLength)
	{
		std::cout << "Data track is not formatted 2048 bytes per sector." << std::endl;
		return 1;
	}

	std::cout << "Input CUE: " << input.fName << std::endl;
	std::cout << "Input BIN: " << input.binFName << std::endl;

	std::string outFNameBase=cpputil::RemoveExtension(av[2]);
	std::string outCUEFName=outFNameBase+".CUE";
	std::string outBINFName=outFNameBase+".BIN";

	std::cout << "Output CUE: " << outCUEFName << std::endl;
	std::cout << "Output BIN: " << outBINFName << std::endl;



	std::ifstream inputCUE,inputBIN;
	inputCUE.open(input.fName);
	inputBIN.open(input.binFName,std::ios::binary);

	std::ofstream outCUE,outBIN;
	outCUE.open(outCUEFName);
	outBIN.open(outBINFName,std::ios::binary);

	if(true!=outCUE.is_open() || true!=outBIN.is_open())
	{
		std::cout << "Cannot open output file." << std::endl;
		return 1;
	}



	// first flag has been checked above.  Safe to access GetTracks()[0].
	auto dataTrack=input.GetTracks()[0];
	auto LBA0=dataTrack.start.ToHSG();
	auto LBA1=dataTrack.end.ToHSG();
	auto numSectorsTrack1=(LBA1-LBA0)+1;

	// Already checked 2048 bytes/sector
	{
		std::vector <char> databuf;
		databuf.resize(numSectorsTrack1*2048);
		inputBIN.read(databuf.data(),databuf.size());
		outBIN.write(databuf.data(),databuf.size());

		auto diff=(2352-2048)*(numSectorsTrack1+input.GetTracks()[1].preGap.ToHSG());
		std::cout << "Padding between data and audio tracks: " << diff << std::endl;

		databuf.resize(diff);
		for(auto &a : databuf)
		{
			a=0;
		}
		outBIN.write(databuf.data(),databuf.size());


		databuf.resize(64*1024*1024);
		for(auto &a : databuf)
		{
			a=0;
		}

		while(true!=inputBIN.eof())
		{
			inputBIN.read(databuf.data(),databuf.size());
			auto actuallyRead=inputBIN.gcount();
			outBIN.write(databuf.data(),actuallyRead);
		}
	}


	inputCUE.close();
	inputBIN.close();




	std::string outBINPath,outBINFile;
	cpputil::SeparatePathFile(outBINPath,outBINFile,outBINFName);

	outCUE << "FILE \"" << outBINFile << "\" BINARY" << std::endl;
	int trackNum=1;
	for(auto trk : input.GetTracks())
	{
		outCUE << "  TRACK ";
		outCUE << cpputil::UitoaZeroPad(trackNum,2) << " ";
		if(DiscImage::TRACK_MODE1_DATA==trk.trackType)
		{
			outCUE << "MODE1/2048" << std::endl;
		}
		else
		{
			outCUE << "AUDIO" << std::endl;
		}

		if(DiscImage::MinSecFrm::Zero()!=trk.preGap)
		{
			outCUE << "    PREGAP ";
			outCUE << cpputil::UitoaZeroPad(trk.preGap.min,2) << ":";
			outCUE << cpputil::UitoaZeroPad(trk.preGap.sec,2) << ":";
			outCUE << cpputil::UitoaZeroPad(trk.preGap.frm,2) << std::endl;
		}
		if(DiscImage::MinSecFrm::Zero()!=trk.index00)
		{
			outCUE << "    INDEX 00 ";
			outCUE << cpputil::UitoaZeroPad(trk.index00.min,2) << ":";
			outCUE << cpputil::UitoaZeroPad(trk.index00.sec,2) << ":";
			outCUE << cpputil::UitoaZeroPad(trk.index00.frm,2) << std::endl;
		}
		outCUE << "    INDEX 01 ";
		outCUE << cpputil::UitoaZeroPad(trk.start.min,2) << ":";
		outCUE << cpputil::UitoaZeroPad(trk.start.sec,2) << ":";
		outCUE << cpputil::UitoaZeroPad(trk.start.frm,2) << std::endl;
		if(DiscImage::MinSecFrm::Zero()!=trk.postGap)
		{
			outCUE << "    INDEX 00 ";
			outCUE << cpputil::UitoaZeroPad(trk.postGap.min,2) << ":";
			outCUE << cpputil::UitoaZeroPad(trk.postGap.sec,2) << ":";
			outCUE << cpputil::UitoaZeroPad(trk.postGap.frm,2) << std::endl;
		}

		trackNum++;
	}
	return 0;
}
