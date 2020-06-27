#include "profiledlg.h"



void ProfileDialog::Make(void)
{
	const int nShowPath=80;

	ROMDirBtn=AddTextButton(0,FSKEY_NULL,FSGUI_PUSHBUTTON,"ROM dir:",YSTRUE);
	ROMDirTxt=AddTextBox(0,FSKEY_NULL,FsGuiTextBox::HORIZONTAL,"",nShowPath,YSFALSE);
	AddStaticText(0,FSKEY_NULL,"(When browsing for the ROM dir, please select one of the ROM files.)",YSTRUE);

	CDImgBtn=AddTextButton(0,FSKEY_NULL,FSGUI_PUSHBUTTON,"CD Image:",YSTRUE);
	CDImgTxt=AddTextBox(0,FSKEY_NULL,FsGuiTextBox::HORIZONTAL,"",nShowPath,YSFALSE);

	FDImgBtn[0]=AddTextButton(0,FSKEY_NULL,FSGUI_PUSHBUTTON,"FD0:",YSTRUE);
	FDImgTxt[0]=AddTextBox(0,FSKEY_NULL,FsGuiTextBox::HORIZONTAL,"",nShowPath,YSFALSE);

	FDImgBtn[1]=AddTextButton(0,FSKEY_NULL,FSGUI_PUSHBUTTON,"FD1:",YSTRUE);
	FDImgTxt[1]=AddTextBox(0,FSKEY_NULL,FsGuiTextBox::HORIZONTAL,"",nShowPath,YSFALSE);

	HDImgBtn[0]=AddTextButton(0,FSKEY_NULL,FSGUI_PUSHBUTTON,"HD0:",YSTRUE);
	HDImgTxt[0]=AddTextBox(0,FSKEY_NULL,FsGuiTextBox::HORIZONTAL,"",nShowPath,YSFALSE);

	HDImgBtn[1]=AddTextButton(0,FSKEY_NULL,FSGUI_PUSHBUTTON,"HD1:",YSTRUE);
	HDImgTxt[1]=AddTextBox(0,FSKEY_NULL,FsGuiTextBox::HORIZONTAL,"",nShowPath,YSFALSE);

	HDImgBtn[2]=AddTextButton(0,FSKEY_NULL,FSGUI_PUSHBUTTON,"HD2:",YSTRUE);
	HDImgTxt[2]=AddTextBox(0,FSKEY_NULL,FsGuiTextBox::HORIZONTAL,"",nShowPath,YSFALSE);

	HDImgBtn[3]=AddTextButton(0,FSKEY_NULL,FSGUI_PUSHBUTTON,"HD3:",YSTRUE);
	HDImgTxt[3]=AddTextBox(0,FSKEY_NULL,FsGuiTextBox::HORIZONTAL,"",nShowPath,YSFALSE);

	HDImgBtn[4]=AddTextButton(0,FSKEY_NULL,FSGUI_PUSHBUTTON,"HD4:",YSTRUE);
	HDImgTxt[4]=AddTextBox(0,FSKEY_NULL,FsGuiTextBox::HORIZONTAL,"",nShowPath,YSFALSE);

	HDImgBtn[5]=AddTextButton(0,FSKEY_NULL,FSGUI_PUSHBUTTON,"HD5:",YSTRUE);
	HDImgTxt[5]=AddTextBox(0,FSKEY_NULL,FsGuiTextBox::HORIZONTAL,"",nShowPath,YSFALSE);

	HDImgBtn[6]=AddTextButton(0,FSKEY_NULL,FSGUI_PUSHBUTTON,"HD6:",YSTRUE);
	HDImgTxt[6]=AddTextBox(0,FSKEY_NULL,FsGuiTextBox::HORIZONTAL,"",nShowPath,YSFALSE);

	AddStaticText(0,FSKEY_NULL,"Game Port 0:",YSTRUE);
	gamePortBtn[0][0]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"None",YSFALSE);
	gamePortBtn[0][1]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"GamePad0",YSFALSE);
	gamePortBtn[0][2]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"GamePad1",YSFALSE);
	gamePortBtn[0][3]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"Emulation by Keyboard(ASZX and arrow)",YSFALSE);
	gamePortBtn[0][4]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"Mouse",YSFALSE);
	SetRadioButtonGroup(5,gamePortBtn[0]);

	AddStaticText(0,FSKEY_NULL,"Game Port 1:",YSTRUE);
	gamePortBtn[1][0]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"None",YSFALSE);
	gamePortBtn[1][1]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"GamePad0",YSFALSE);
	gamePortBtn[1][2]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"GamePad1",YSFALSE);
	gamePortBtn[1][3]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"Emulation by Keyboard(ASZX and arrow)",YSFALSE);
	gamePortBtn[1][4]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"Mouse",YSFALSE);
	SetRadioButtonGroup(5,gamePortBtn[1]);

	AddStaticText(0,FSKEY_NULL,"Boot Option:",YSTRUE);
	bootKeyBtn[ 0]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"None",YSFALSE);
	bootKeyBtn[ 1]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"CD",YSFALSE);
	bootKeyBtn[ 2]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"F0",YSFALSE);
	bootKeyBtn[ 3]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"F1",YSFALSE);
	bootKeyBtn[ 4]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"F2",YSFALSE);
	bootKeyBtn[ 5]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"F3",YSFALSE);
	bootKeyBtn[ 6]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"H0",YSFALSE);
	bootKeyBtn[ 7]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"H1",YSFALSE);
	bootKeyBtn[ 8]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"H2",YSFALSE);
	bootKeyBtn[ 9]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"H3",YSFALSE);
	bootKeyBtn[10]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"H4",YSFALSE);
	bootKeyBtn[11]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"ICM",YSFALSE);
	bootKeyBtn[12]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"DEBUG",YSFALSE);
	bootKeyBtn[13]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"PAD_A",YSFALSE);
	bootKeyBtn[14]=AddTextButton(0,FSKEY_NULL,FSGUI_RADIOBUTTON,"PAD_B",YSFALSE);
	SetRadioButtonGroup(sizeof(bootKeyBtn)/sizeof(bootKeyBtn[0]),bootKeyBtn);

	runBtn=AddTextButton(0,FSKEY_NULL,FSGUI_PUSHBUTTON,"START",YSTRUE);
	autoStartBtn=AddTextButton(0,FSKEY_NULL,FSGUI_CHECKBOX,"Auto Start (Applicable to Default Profile only)",YSFALSE);

	Fit();
	SetArrangeType(FSDIALOG_ARRANGE_TOP_LEFT);
}

/* virtual */ void ProfileDialog::OnButtonClick(FsGuiButton *btn)
{
	if(ROMDirBtn==btn)
	{
		YsString def;
		def.MakeFullPathName(ROMDirTxt->GetString(),"FMT_SYS.ROM");

		auto fdlg=FsGuiDialog::CreateSelfDestructiveDialog<FsGuiFileDialog>();
		fdlg->Initialize();
		fdlg->mode=FsGuiFileDialog::MODE_OPEN;
		fdlg->multiSelect=YSFALSE;
		fdlg->title.Set(L"Select a ROM file (like FMT_SYS.ROM)");
		fdlg->fileExtensionArray.Append(L".ROM");
		fdlg->defaultFileName.SetUTF8String(def);
		fdlg->BindCloseModalCallBack(&THISCLASS::OnSelectROMFile,this);
		AttachModalDialog(fdlg);
	}
	if(CDImgBtn==btn)
	{
		Browse(L"CD Image",CDImgTxt,L".CUE",L".ISO");
	}
	for(int i=0; i<sizeof(FDImgBtn)/sizeof(FDImgBtn[0]); ++i)
	{
		if(FDImgBtn[i]==btn)
		{
			YsWString label(L"Floppy Drive ");
			label.push_back('0'+i);
			Browse(label,FDImgTxt[i],L".BIN",nullptr); // ,L".D77"
		}
	}
	for(int i=0; i<sizeof(HDImgBtn)/sizeof(HDImgBtn[0]); ++i)
	{
		if(HDImgBtn[i]==btn)
		{
			YsWString label(L"SCSI HDD ");
			label.push_back('0'+i);
			Browse(label,HDImgTxt[i],L".BIN",L".HDD");
		}
	}
}
void ProfileDialog::OnSelectROMFile(FsGuiDialog *dlg,int returnCode)
{
	auto fdlg=dynamic_cast <FsGuiFileDialog *>(dlg);
	if((int)YSOK==returnCode && nullptr!=fdlg)
	{
		YsWString ful(fdlg->selectedFileArray[0]);
		YsWString pth,fil;
		ful.SeparatePathFile(pth,fil);
		ROMDirTxt->SetText(pth);
	}
}
void ProfileDialog::Browse(const wchar_t label[],FsGuiTextBox *txt,const wchar_t ext0[],const wchar_t ext1[])
{
	nowBrowsingTxt=txt;

	YsString def=ROMDirTxt->GetString();

	auto fdlg=FsGuiDialog::CreateSelfDestructiveDialog<FsGuiFileDialog>();
	fdlg->Initialize();
	fdlg->mode=FsGuiFileDialog::MODE_OPEN;
	fdlg->multiSelect=YSFALSE;
	fdlg->title.Set(label);
	fdlg->fileExtensionArray.Append(ext0);
	if(nullptr!=ext1)
	{
		fdlg->fileExtensionArray.Append(ext1);
	}
	fdlg->defaultFileName.SetUTF8String(def);
	fdlg->BindCloseModalCallBack(&THISCLASS::OnSelectFile,this);
	AttachModalDialog(fdlg);
}
void ProfileDialog::OnSelectFile(FsGuiDialog *dlg,int returnCode)
{
	auto fdlg=dynamic_cast <FsGuiFileDialog *>(dlg);
	if((int)YSOK==returnCode && nullptr!=fdlg)
	{
		nowBrowsingTxt->SetText(fdlg->selectedFileArray[0]);
	}
}

TownsProfile ProfileDialog::GetProfile(void) const
{
	TownsProfile profile;

	profile.ROMDir=ROMDirTxt->GetString().data();
	profile.CDImgFile=CDImgTxt->GetString().data();
	profile.FDImgFile[0][0]=FDImgTxt[0]->GetString().data();
	profile.FDImgFile[1][0]=FDImgTxt[1]->GetString().data();
	profile.SCSIImgFile[0]=HDImgTxt[0]->GetString().data();
	profile.SCSIImgFile[1]=HDImgTxt[1]->GetString().data();
	profile.SCSIImgFile[2]=HDImgTxt[2]->GetString().data();
	profile.SCSIImgFile[3]=HDImgTxt[3]->GetString().data();
	profile.SCSIImgFile[4]=HDImgTxt[4]->GetString().data();
	profile.SCSIImgFile[5]=HDImgTxt[5]->GetString().data();
	profile.bootKeyComb=BOOT_KEYCOMB_NONE;
	for(int i=0; i<sizeof(bootKeyBtn)/sizeof(bootKeyBtn[0]); ++i)
	{
		if(YSTRUE==bootKeyBtn[i]->GetCheck())
		{
			profile.bootKeyComb=i;
			break;
		}
	}
	profile.autoStart=(YSTRUE==autoStartBtn->GetCheck());

	return profile;
}
void ProfileDialog::SetProfile(const TownsProfile &profile)
{
	YsWString str;

	str.SetUTF8String(profile.ROMDir.data());
	ROMDirTxt->SetText(str);

	str.SetUTF8String(profile.CDImgFile.data());
	CDImgTxt->SetText(str);

	str.SetUTF8String(profile.FDImgFile[0][0].data());
	FDImgTxt[0]->SetText(str);

	str.SetUTF8String(profile.FDImgFile[1][0].data());
	FDImgTxt[1]->SetText(str);

	for(int i=0; i<TownsProfile::MAX_NUM_SCSI_DEVICE; ++i)
	{
		str.SetUTF8String(profile.SCSIImgFile[i].data());
		HDImgTxt[i]->SetText(str);
	}

	for(int i=0; i<sizeof(bootKeyBtn)/sizeof(bootKeyBtn[0]); ++i)
	{
		if(i==profile.bootKeyComb)
		{
			bootKeyBtn[i]->SetCheck(YSTRUE);
		}
		else
		{
			bootKeyBtn[i]->SetCheck(YSFALSE);
		}
	}

	if(true==profile.autoStart)
	{
		autoStartBtn->SetCheck(YSTRUE);
	}
	else
	{
		autoStartBtn->SetCheck(YSFALSE);
	}
}