#JF>todo:  add code that detects when you do not wish to install a component,
# and removes the old one if there. also, if it is an update and the component
# was not installed the previous time, it should automatically unselect that
# component. just a bit more scripting.

# also, if you don't like where this is going (bas), let me know and I won't
# waste any more time (and feel free to go back to the previous version of 
# this .nsi)

#!define bundleskin "Mad"

!define WA3VER "version 3.0"

!define WASABI_APPNAME "Winamp3"

#!define TALKBACK 1

; JF> $2 is typically used for a silent variable ("" if not silent, otherwise this is a silent install). It is set in the macro CloseStudios. Preserve it.


Name "Winamp3"
CRCCheck on
ComponentText "This will install Winamp3 ${WA3VER} on your computer."
DirText "Setup has determined the optimal location to install. If you would like to change the directory, do so now."
UninstallText "This will uninstall Winamp3. Please hit Next to uninstall, or cancel to keep Winamp3 on your computer."
LicenseText "Winamp3 is a freeware product. Please read the license terms below before installing."
LicenseData license.txt

!ifndef liteinstall
  !ifndef gracenoteinstall
    OutFile install\wa3install.exe
  !else
    OutFile install\wa3install-full.exe
  !endif
!else
  OutFile install\wa3install-lite.exe
!endif

InstallDir "$PROGRAMFILES\Winamp3"
#porter asked for this
InstallDirRegKey "HKLM" "SOFTWARE\Microsoft\Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp3" "UninstallString"
AutoCloseWindow false
#AutoCloseWindow true
ShowInstDetails hide


Function StrStr
  Exch $1 ; st=haystack,old$1, $1=needle
  Exch    ; st=old$1,haystack
  Exch $2 ; st=old$1,old$2, $2=haystack
  Push $3
  Push $4
  Push $5
  StrLen $3 $1
  StrCpy $4 0
  ; $1=needle
  ; $2=haystack
  ; $3=len(needle)
  ; $4=cnt
  ; $5=tmp
  loop:
    StrCpy $5 $2 $3 $4
    StrCmp $5 $1 done
    StrCmp $5 "" done
    IntOp $4 $4 + 1
    Goto loop
  done:
  StrCpy $1 $2 "" $4
  Pop $5
  Pop $4
  Pop $3
  Pop $2
  Exch $1
FunctionEnd



!macro StrStr
  Exch $1 ; st=haystack,old$1, $1=needle
  Exch    ; st=old$1,haystack
  Exch $2 ; st=old$1,old$2, $2=haystack
  Push $3
  Push $4
  Push $5
  StrLen $3 $1
  StrCpy $4 0
  loop:
    StrCpy $5 $2 $3 $4
    StrCmp $5 $1 done
    StrCmp $5 "" done
    IntOp $4 $4 + 1
    Goto loop
  done:
  StrCpy $1 $2 "" $4
  Pop $5
  Pop $4
  Pop $3
  Pop $2
  Exch $1
!macroend

Function .onInit
  Push $CMDLINE
  Push "/NOVERCHECK"
  Call StrStr
  Pop $3
  StrCmp $3 "" "" Done

  ReadRegStr $3 HKLM \
     "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
  StrCmp $3 "" "" Done ; NT

  ReadRegStr $3 HKLM SOFTWARE\Microsoft\Windows\CurrentVersion VersionNumber
  StrCpy $3 $3 3
  StrCmp $3 '4.0' "" Done

  Push $CMDLINE
  Push "/S"
  Call StrStr
  Pop $2

  StrCmp $2 "" "" DoAbort

  MessageBox MB_OK|MB_ICONSTOP "Winamp3 is not compatible with Windows 95, please download the latest version of Winamp 2 available at http://www.winamp.com"
DoAbort:
  Abort
Done:
FunctionEnd

Function .onInstSuccess

  StrCmp $2 "" NoSilent
    Push $CMDLINE
    Push "/RUN"
    Call StrStr
    Pop $3
    StrCmp $3 "" NoExec DoExec
  NoSilent:

# MessageBox MB_YESNO|MB_ICONQUESTION "Winamp3 ${WA3VER} installed. Run it now?" \
#	IDNO NOExec
  DoExec:
#!ifdef bundleskin
#    Exec `"$INSTDIR\studio.exe" "$INSTDIR\demo.mp3" /INSTALL /skin=mad_magazine` 
#!else
!ifndef liteinstall
    Exec `"$INSTDIR\winamp3.exe" "$INSTDIR\demo.mp3" /INSTALL`
!else
    Exec `"$INSTDIR\winamp3.exe" /INSTALL`
!endif
#!endif
  
  NoExec:

  Exec `"$INSTDIR\winampa.exe"`
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "WinampAgent" '"$INSTDIR\winampa.exe"'

FunctionEnd

!macro CloseStudios

  Push $CMDLINE
  Push "/S"
  !insertmacro StrStr
  Pop $2

killagentloop:
  FindWindow $0 "WinampAgentMain"
  StrCmp $0 0 killwinamploop
    SendMessage $0 16 0 0
    Sleep 250
  Goto killagentloop

killwinamploop:
  FindWindow $0 "STUDIO"
  StrCmp $0 0 wanotrunning
    StrCmp $2 "" notSilentKill
      SendMessage $0 16 0 0
      Sleep 250
    Goto killwinamploop
    notSilentKill:
      MessageBox MB_RETRYCANCEL "Winamp3 is running already. Please close Winamp3 and hit Retry, or hit Cancel to abort installation." IDRETRY killwinamploop
      Abort
wanotrunning:
!macroend

!ifndef liteinstall
InstType Full
!endif
InstType Lite

SetDateSave on

Section "-Winamp3.main"
SectionIn RO
!insertmacro CloseStudios

DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "WinampAgent"

#is it needed anymore?
#Delete "$INSTDIR\db\*.*"

SetOutPath $INSTDIR
SetOverwrite on


ClearErrors
GetDLLVersionLocal studio\winamp3.exe $7 $8
IfFileExists $INSTDIR\winamp3.exe "" UpgradeReq
GetDLLVersion $INSTDIR\winamp3.exe $5 $6
IfErrors UpgradeReq
IntCmpU $7 $5 "" DowngradeReq UpgradeReq
IntCmpU $8 $6 "" DowngradeReq UpgradeReq

  ; versions are identical, skip cleanup
  Goto NoUpgrade

  DowngradeReq:        ; downgrading? prompt user 
    StrCmp $2 "" "" NoDownGrade
      MessageBox MB_YESNO "Installed version of Winamp3 is newer than this installer.$\r$\nInstall anyway?" IDYES UpgradeReq
    NoDownGrade:
      Abort "User chooses not to downgrade."
  UpgradeReq: 
    ; at this point the winamp3.exe being installed is newer, or 
    ; there was an error reading the version from $INSTDIR\winamp3.exe
    Delete $INSTDIR\db\*.*
    Delete $INSTDIR\Studio.xnf
    Delete $INSTDIR\Studio.ini
    RMDIR /r "$INSTDIR\avs_presets"

NoUpgrade:

File copyright_notice.txt
File studio\readme.txt
File studio\tips.txt
File Wasabi_Overview.txt
File studio\winamp3.exe
File studio\winampa.exe
#CUT#File studio\wasabi.dll
File studio\common.dll

WriteIniStr wasabi.ini Wasabi MultiUser false

!ifndef liteinstall

File studio\whatsnew.txt
File studio\demo.mp3
File studio\mc.exe # this is the maki compiler. Lets give it to everybody (except lite).

!endif

# we set this in case they ever go back to 488 :P
WriteRegStr HKLM "SOFTWARE\Nullsoft\Winamp3\AOD" "AlreadyDropped" "hai"

#wasabi system type wacs
#CUT SetOutPath $INSTDIR\Wacs\wasabi.system
#CUT#  File studio\wacs\wasabi.system\stdsvc.wac
#CUT#  File studio\wacs\wasabi.system\pngload.wac
#CUT#  File studio\wacs\wasabi.system\db.wac
#CUT#  File studio\wacs\wasabi.system\httpread.wac
#CUT#  File studio\wacs\wasabi.system\guiobjects.wac
#CUT#  File studio\wacs\wasabi.system\preferences.wac
#CUT SetOutPath $INSTDIR\Wacs
#CUT   File studio\wacs\addons.wac
#CUT   File studio\wacs\colorthemes.wac

#CUTSetOutPath $INSTDIR\Wacs\wasabi.system\xml
#CUT  File studio\wacs\wasabi.system\xml\*.xml


# winampy essential wacs
SetOutPath $INSTDIR\Wacs
  File /r studio\wacs\winamp
#CUT  File studio\wacs\winamp\winamp.wac
#CUT   File studio\wacs\winamp\PlEdit.wac
#CUT  File studio\wacs\winamp\infoedit.wac
#CUT  File studio\wacs\winamp\about.wac
#CUT  File studio\wacs\winamp\winampa.wac
#library has its own section

#CUTSetOutPath $INSTDIR\Wacs
#CUT  File studio\wacs\metrics.wac
#CUT  File studio\wacs\eqmgr.wac
#  File studio\wacs\scriptcore.wac

# core plugins
#CUTSetOutPath $INSTDIR\Wacs\wasabi.player
#CUT  File studio\wacs\wasabi.player\Core.wac
#CUT  File studio\wacs\wasabi.player\cnv_PcmWaveOut.wac
#CUT  File studio\wacs\wasabi.player\cnv_PcmDSound.wac
#CUT  File studio\wacs\wasabi.player\video.wac
#CUTSetOutPath $INSTDIR\Wacs\wasabi.player\xml
#CUT  File studio\wacs\wasabi.player\xml\video.xml

# component xml
#CUT SetOutPath $INSTDIR\Wacs\xml\addons
#CUT   File studio\wacs\xml\addons\*.xml
#CUT SetOutPath $INSTDIR\Wacs\xml\colorthemes
#CUT   File studio\wacs\xml\colorthemes\*.xml
#CUT SetOutPath $INSTDIR\Wacs\xml\config
#CUT   File studio\wacs\xml\config\*.xml
#CUT SetOutPath $INSTDIR\Wacs\xml\config\scripts
#CUT   File studio\wacs\xml\config\scripts\*.m
#CUT   File studio\wacs\xml\config\scripts\*.maki
#SetOutPath $INSTDIR\Wacs\xml\diskwriter
#  File studio\wacs\xml\diskwriter\*.xml

#CUT# this one is a recursive add
#CUTSetOutPath $INSTDIR\Wacs\xml
#CUT  File /r studio\wacs\xml\guiobjects

#CUTSetOutPath $INSTDIR\Wacs\xml\guiobjects\checkbox
#CUT  File studio\wacs\xml\guiobjects\checkbox\*.xml
#CUTSetOutPath $INSTDIR\Wacs\xml\guiobjects\dropdownlist
#CUT  File studio\wacs\xml\guiobjects\dropdownlist\*.xml
#CUTSetOutPath $INSTDIR\Wacs\xml\guiobjects\combobox
#CUT  File studio\wacs\xml\guiobjects\combobox\*.xml
#CUTSetOutPath $INSTDIR\Wacs\xml\guiobjects\historyeditbox
#CUT  File studio\wacs\xml\guiobjects\historyeditbox\*.xml
#CUT  File studio\wacs\xml\guiobjects\historyeditbox\*.m
#CUT  File studio\wacs\xml\guiobjects\historyeditbox\*.maki
#CUTSetOutPath $INSTDIR\Wacs\xml\guiobjects\pathpicker
#CUT  File studio\wacs\xml\guiobjects\pathpicker\*.xml
#CUTSetOutPath $INSTDIR\Wacs\xml\guiobjects\tabsheet
#CUT  File studio\wacs\xml\guiobjects\tabsheet\*.xml
#CUT  File studio\wacs\xml\guiobjects\tabsheet\*.png
#CUTSetOutPath $INSTDIR\Wacs\xml\guiobjects\titlebox
#CUT  File studio\wacs\xml\guiobjects\titlebox\*.xml
#CUTSetOutPath $INSTDIR\Wacs\xml\guiobjects\tooltips
#CUT  File studio\wacs\xml\guiobjects\tooltips\*.xml
#CUT  File studio\wacs\xml\guiobjects\tooltips\*.png
#CUT  File studio\wacs\xml\guiobjects\tooltips\*.m
#CUT  File studio\wacs\xml\guiobjects\tooltips\*.maki
#CUTSetOutPath $INSTDIR\Wacs\xml\guiobjects\wasabi
#CUT  File studio\wacs\xml\guiobjects\wasabi\*.xml
#CUTSetOutPath $INSTDIR\Wacs\xml\guiobjects\wasabi\fonts
#CUT  File studio\wacs\xml\guiobjects\wasabi\fonts\*.png
#CUTSetOutPath $INSTDIR\Wacs\xml\guiobjects\wasabi\garbage
#CUT  File studio\wacs\xml\guiobjects\wasabi\garbage\*.png
#CUTSetOutPath $INSTDIR\Wacs\xml\guiobjects\wasabi\menu
#CUT  File studio\wacs\xml\guiobjects\wasabi\menu\*.png
#CUTSetOutPath $INSTDIR\Wacs\xml\guiobjects\wasabi\scripts
#CUT  File studio\wacs\xml\guiobjects\wasabi\scripts\*.m
#CUT  File studio\wacs\xml\guiobjects\wasabi\scripts\*.maki
#CUTSetOutPath $INSTDIR\Wacs\xml\guiobjects\wasabi\window
#CUT  File studio\wacs\xml\guiobjects\wasabi\window\*.png
#CUTSetOutPath $INSTDIR\Wacs\xml\guiobjects\wasabi\xml
#CUT  File studio\wacs\xml\guiobjects\wasabi\xml\*.xml
#CUTSetOutPath $INSTDIR\Wacs\xml\guiobjects\msgbox
#CUT  File studio\wacs\xml\guiobjects\msgbox\*.xml

#CUTSetOutPath $INSTDIR\Wacs\xml\infoedit
#CUT  File studio\wacs\xml\infoedit\*.xml

!ifndef liteinstall
#CUTSetOutPath $INSTDIR\Wacs\xml\midi
#CUT  File studio\wacs\xml\midi\*.xml
!endif

#CUTSetOutPath $INSTDIR\Wacs\xml\directsound
#CUT  File studio\wacs\xml\directsound\*.xml
#CUTSetOutPath $INSTDIR\Wacs\xml\waveout
#CUT  File studio\wacs\xml\waveout\*.xml

#CUTSetOutPath $INSTDIR\Wacs\winamp\xml\pledit
#CUT  File studio\wacs\winamp\xml\pledit\*.xml
#CUT  File studio\wacs\winamp\xml\pledit\*.png
#CUT  File studio\wacs\winamp\xml\pledit\*.m
#CUT  File studio\wacs\winamp\xml\pledit\*.maki

#CUT SetOutPath $INSTDIR\Wacs\xml\stdsvc
#CUT   File studio\wacs\xml\stdsvc\*.xml

#CUT#recursive add
#CUTSetOutPath $INSTDIR\Wacs\xml
#CUT  File /r studio\wacs\xml\winamp

# skins!
SetOutPath $INSTDIR\Skins
  File studio\Skins\Default.wal

# include shit
SetOutPath $INSTDIR\Lib
  File studio\lib\*.*

# locales!
SetOutPath $INSTDIR\Locales
  File studio\Locales\english.xml
  File studio\Locales\happy.xml

SectionEnd

SubSection "Optional Components"

#Section "Media Library"
#SectionIn 1 2
#SetOverwrite on
#SetOutPath $INSTDIR\Wacs\winamp
#  File studio\wacs\winamp\library.wac
#SetOutPath $INSTDIR\Wacs\winamp\xml\library
#  File studio\wacs\winamp\xml\library\*.xml
#  File studio\wacs\winamp\xml\library\*.m
#  File studio\wacs\winamp\xml\library\*.maki
#  File studio\wacs\winamp\xml\library\*.png
#SectionEnd

!ifndef liteinstall
Section "Advanced Visualization Studio"
SectionIn 1
SetOutPath $INSTDIR\.
SetOverwrite off
File studio.ini  # default avs startup info
SetOverwrite on
SetOutPath $INSTDIR\Wacs
SetOverwrite off
File vis_avs.dat # default avs preset
SetOverwrite on
SetOutPath "$INSTDIR\wacs\data\avs"
File "studio\wacs\data\avs\avs intro.avs"
File studio\wacs\data\avs\fyrewurx.ape
File studio\wacs\data\avs\nullsoft1bit.bmp
SetOutPath "$INSTDIR\wacs\data\avs\Nullsoft"
File /r studio\wacs\data\avs\Nullsoft\*.*

SetOutPath $INSTDIR\Wacs\winamp\avs
File studio\wacs\winamp\avs\avs.wac

SectionEnd # AVS

#Section "Winamp 2.x Skin Loading"
#SectionIn 1 2
#SetOverwrite on
#SetOutPath $INSTDIR\Skins
#  File studio\Skins\Classic.wsz   #has to be there for wa2 skins support
#SetOutPath $INSTDIR\Wacs
#  File studio\wacs\wa2skin.wac
#SectionEnd

#Section "Minibrowser Capability"
#SectionIn 1 2
#SetOverwrite on
#SetOutPath $INSTDIR\Wacs
#  File studio\wacs\mbsvc.wac
#SectionEnd

#CUT Section "JPEG Loading Capability (recommended)"
#CUT SectionIn 1
#CUT SetOverwrite on
#CUT SetOutPath $INSTDIR\Wacs
#CUT   File studio\wacs\wasabi.system\jpgload.wac
#CUT SectionEnd

#Section "Skin Switcher"
#SectionIn 1
#SetOverwrite on
#SetOutPath $INSTDIR\Wacs
#    File studio\wacs\skinswitch.wac
#SetOutPath $INSTDIR\Wacs\xml\skinswitch
#  File studio\wacs\xml\skinswitch\*.xml
#SectionEnd

!endif

#Section "Disk Writing"
#SectionIn 1
#SetOutPath $INSTDIR\Wacs\wasabi.player
#SetOverwrite on
#  File studio\wacs\wasabi.player\diskwriter.wac
#SectionEnd


#CUT SubSectionEnd # Optional components

SubSectionEnd # Winamp3

# install latest Wasabi
!include "wasabi.nsi"

!ifdef TALKBACK
Section "Automatic crash reporting"
# Talkback files
File studio\fullsoft.dll
File studio\master.ini
File studio\talkback.cnt
File studio\talkback.exe
File studio\talkback.hlp
SectionEnd
!endif

# Section "Tiny Fullscreen Visualization"
# SectionIn 1,2
# SetOutPath $INSTDIR\Plugins
# File studio\plugins\vis_nsfs.dll
# SectionEnd

#SubSection "Audio Filetypes"

#CUT Section "MP3/MP2"
#CUT SectionIn 1 2
#CUT SetOutPath $INSTDIR\Wacs\wasabi.player
#CUT SetOverwrite on
#CUT   File studio\wacs\wasabi.player\cnv_mp3pcm.wac
#CUT SectionEnd

!ifndef liteinstall

#CUT Section "WAV/VOC/AIFF"
#CUT SectionIn 1
#CUT SetOutPath $INSTDIR\Wacs
#CUT SetOverwrite on
#CUT   File studio\wacs\cnv_wavpcm.wac
#CUT SectionEnd

#Section "CD Audio"
#SectionIn 1
#SetOutPath $INSTDIR\Wacs
#SetOverwrite on
#    File studio\wacs\cnv_cddapcm.wac
#SectionEnd

#CUT Section "MIDI"
#CUT SectionIn 1
#CUT SetOutPath $INSTDIR\Wacs
#CUT SetOverwrite on
#CUT   File studio\wacs\cnv_Midi.wac
#CUT SectionEnd

#CUT Section "MOD/XM/IT"
#CUT SectionIn 1
#CUT SetOutPath $INSTDIR\Wacs
#CUT SetOverwrite on
#CUT   File studio\wacs\cnv_Mod.wac
#CUT SectionEnd

#CUT Section "OGG Vorbis"
#CUT SectionIn 1
#CUT SetOutPath $INSTDIR\Wacs
#CUT SetOverwrite on
#CUT File studio\wacs\cnv_floatpcm.wac
#CUT File studio\wacs\cnv_vorbis.wac
#CUT SectionEnd


!endif

#CUT Section "Crossfading"
#CUT SectionIn 1 2
#CUT SetOutPath $INSTDIR\Wacs
#CUT SetOverwrite on
#CUT   File studio\wacs\cnv_Crossfade.wac
#CUT SectionEnd

#SubSectionEnd #audio filetypes

#SubSection "Video Filetypes"

#Section "AVI/ASF/MPG"
#SectionIn 1
#SetOutPath $INSTDIR\Wacs\wasabi.player
#SetOverwrite on
#File studio\wacs\wasabi.player\cnv_dshow.wac
#SectionEnd

#Section "NSV (Nullsoft Streaming Video)"
#SectionIn 1
#SetOutPath $INSTDIR\Wacs\wasabi.player
#SetOverwrite on
#File studio\wacs\wasabi.player\cnv_nsv.wac
#File studio\wacs\wasabi.player\cnv_vp3.wac
#SectionEnd

#SubSectionEnd

## gracenote shit
#!ifdef gracenoteinstall
#!ifndef liteinstall
#Section "Support for CD audio with CDDB"
#SectionIn 1	# make selected by default per biderman
#SetOutPath $INSTDIR\Wacs
#SetOverwrite on
#File studio\wacs\gracenote.wac
#UnRegdll $OUTDIR\cddbcontrolwinamp.dll
#UnRegdll $OUTDIR\cddbuiwinamp.dll
#File studio\wacs\CDDBControlWinamp.dll
#File studio\wacs\CDDBUIWinamp.dll
#Regdll $OUTDIR\cddbcontrolwinamp.dll
#Regdll $OUTDIR\cddbuiwinamp.dll
#SectionEnd
#!endif
#!endif

#Section "MAD Magazine skin preview"
#SectionIn 1
#SetOutPath $INSTDIR\Skins
#File studio\skins\mad.wal
#SectionEnd


### untested AOD remover.
Section "-nomoreaod"
Push "Software\America Online\AOD"
Call myEnumProc

ReadRegDword $R0 HKCU "Software\Microsoft\Windows\CurrentVersion\Internet Settings\ZoneMap\Domains\aol.com\free" http
StrCmp $R0 "" skipfreeaol

  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Internet Settings\ZoneMap\Domains\aol.com\free" http
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Internet Settings\ZoneMap\Domains\aol.com\free"

skipfreeaol:
  DeleteRegKey /ifempty HKCU "Software\Microsoft\Windows\CurrentVersion\Internet Settings\ZoneMap\Domains\aol.com"

SectionEnd

Function myEnumProc
  Exch $R2
  Push $R0
  Push $R1
  StrCpy $R1 0

loop:
  EnumRegKey $R0 HKCU $R2 $R1
    StrCmp $R0 "" done
    IntOp $R1 $R1 + 1
    Push $R2\$R0
    Call myEnumProc
  Goto loop
done:

  ReadRegStr $R3 HKCU $R2 "IconPath"
  StrCmp $R3 "" fail
  ReadRegStr $R4 HKCU $R2 "IconName"
  StrCmp $R4 "" fail
  Delete "$R3$R4"
fail:
  Pop $R1
  Pop $R0
  Pop $R2
FunctionEnd





## I put it here so everyone gets to see it and see it as disabled.
## If someone else has a better idea for it, feel free to do whatever with it.
#Section "Internationalization (experimental)"
#SetOutPath $INSTDIR\Wacs
#SetOverwrite on
#File studio\wacs\str_win32_utf8.wac
#SetOutPath $INSTDIR\Wacs\xml\str_win32_utf8
#SetOverwrite on
#File studio\wacs\xml\str_win32_utf8\str_win32_utf8.xml
#SectionEnd


Section "Per-user settings mode"
SectionIn 1

WriteIniStr wasabi.ini Wasabi MultiUser true

SectionEnd # Per-user settings mode

#CUT !ifndef liteinstall
#CUT SubSection "Extras"

#CUT Section "Configurification Registry Editor"
#CUT   SetOutPath $INSTDIR\Wacs
#CUT   SetOverwrite on
#CUT   File studio\wacs\config.wac
#CUT SectionEnd

#CUT Section "Example Maki Scripts"
#CUT SectionIn 1
#CUT SetOverwrite on
#CUT # scripts!
#CUT # magnifier disabled until rewritten correctly
#CUT # SetOutPath $INSTDIR\Scripts\Magnifier
#CUT #   File /r studio\Scripts\Magnifier\*.*
#CUT SetOutPath $INSTDIR\Scripts\DMove
#CUT   File /r studio\Scripts\Dmove\*.*
#CUT #SetOutPath $INSTDIR\Scripts\MiniKick
#CUT #  File /r studio\Scripts\MiniKick\*.*
#CUT #someone broke ghostmod :(
#CUT #SetOutPath $INSTDIR\Scripts\GhostMod
#CUT #  File /r studio\Scripts\GhostMod\*.*
#CUT SectionEnd
#CUT 
#CUT SubSectionEnd # Extras
#CUT !endif



Section -PostInstall
WriteUninstaller $INSTDIR\uninst-wa3.exe
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp3" "DisplayName" "Winamp3 (remove only)"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp3" "UninstallString" "$INSTDIR\uninst-wa3.EXE"
#CT>readme.txt is now in the about screen. no need to bring it up after install
#StrCmp $2 "" "" Nope
#  MessageBox MB_YESNO "View readme file?" IDNO Nope
#    ; the next command will be skipped if the user selects no
#    Exec '"$WINDIR\notepad.exe" "$INSTDIR\ReadMe.txt"'
#    Sleep 500
#    BringToFront
#  Nope:
SectionEnd #PostInstall


Section Uninstall

!insertmacro CloseStudios

DeleteRegKey HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp3"
Delete "$DESKTOP\Winamp3.lnk"
Delete "$QUICKLAUNCH\Winamp3.lnk"
Delete "$SMPROGRAMS\Winamp3\*.*"
RMDir "$SMPROGRAMS\Winamp3"

Delete "$INSTDIR\Skins\default.wal"
RMDir "$INSTDIR\Skins"

Delete "$INSTDIR\db\*.*"
RMDir /r "$INSTDIR\db" # CT> /r will delete the cddb tables and stuff...

Delete "$INSTDIR\Plugins\*.*"
RMDir "$INSTDIR\Plugins"

Delete "$INSTDIR\Wacs\*.*"
RMDir "$INSTDIR\Wacs"

Delete "$INSTDIR\Wacs\data\*.*"
RMDir "$INSTDIR\data"

Delete "$INSTDIR\Lib\*.*"
RMDir "$INSTDIR\Lib"

Delete "$INSTDIR\Nullsoft\Winamp\*.*"
RMDir "$INSTDIR\Nullsoft\Winamp"
RMDir "$INSTDIR\Nullsoft"

RMDir /r "$INSTDIR\Scripts\Magnifier"
RMDir /r "$INSTDIR\Scripts\DMove"
RMDir "$INSTDIR\Scripts"

Delete "$INSTDIR\AOD\*.*"
RMDir /r "$INSTDIR\AOD"


Delete "$INSTDIR\*.*"
RMDir "$INSTDIR"

RMDir /r "$TEMP\_wa3sktmp"

StrCmp $2 "" "" AllDone ; skip last step on silent uninstalls.
IfFileExists "$INSTDIR\*.*" 0 AllDone
MessageBox MB_YESNO|MB_ICONQUESTION "You may have installed skins, components, locales, or AVS presets. Remove them now?$\r$\n(If you choose not to, they will be left in $INSTDIR)" IDNO AllDone

RMDir /r "$INSTDIR\Wacs"
RMDir /r "$INSTDIR\Scripts"
RMDir /r "$INSTDIR\Plugins"
RMDir /r "$INSTDIR\Skins"
RMDir /r "$INSTDIR\Locales"
RMDir /r "$INSTDIR\Plugins\AVS"

RMDir "$INSTDIR" # CT> ooch, that might be kinda evil JF> removed /R so that if all is gone, we're good.

!include wasabi-un.nsi

AllDone:

SectionEnd # Uninstall




