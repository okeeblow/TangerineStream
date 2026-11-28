#JF>todo:  add code that detects when you do not wish to install a component,
# and removes the old one if there. also, if it is an update and the component
# was not installed the previous time, it should automatically unselect that
# component. just a bit more scripting.

# also, if you don't like where this is going (bas), let me know and I won't
# waste any more time (and feel free to go back to the previous version of 
# this .nsi)

#!define bundleskin "Mad"

!define WA3VER "version 3.0"

!define WASABI_APPNAME "wasabi.player"

#!define TALKBACK 1

; JF> $2 is typically used for a silent variable ("" if not silent, otherwise this is a silent install). It is set in the macro CloseStudios. Preserve it.


Name "wasabi.player"
CRCCheck on
ComponentText "This will install wasabi.player ${WA3VER} on your computer."
DirText "Setup has determined the optimal location to install. If you would like to change the directory, do so now."
UninstallText "This will uninstall wasabi.player. Please hit Next to uninstall, or cancel to keep wasabi.player on your computer."
LicenseText "wasabi.player is a freeware product. Please read the license terms below before installing."
LicenseData txt\license.txt

!ifndef liteinstall
  !ifndef gracenoteinstall
    OutFile install\wasabi.player.exe
  !else
    OutFile install\wa3install-full.exe
  !endif
!else
  OutFile install\wa3install-lite.exe
!endif

InstallDir "$PROGRAMFILES\wasabi.player"
#porter asked for this
InstallDirRegKey "HKLM" "SOFTWARE\Microsoft\Software\Microsoft\Windows\CurrentVersion\Uninstall\wasabi.player" "UninstallString"

#AutoCloseWindow false
AutoCloseWindow true
;CT> Autoclose needs to be true cause wa3's installer takes over

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

  MessageBox MB_OK|MB_ICONSTOP "wasabi.player is not compatible with Windows 95, please download the latest version of Winamp 2 available at http://www.winamp.com"
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

# MessageBox MB_YESNO|MB_ICONQUESTION "wasabi.player ${WA3VER} installed. Run it now?" \
#	IDNO NOExec
  DoExec:
#!ifdef bundleskin
#    Exec `"$INSTDIR\studio.exe" "$INSTDIR\demo.mp3" /INSTALL /skin=mad_magazine` 
#!else
!ifndef liteinstall
    Exec `"$INSTDIR\wasabi.exe" "$INSTDIR\demo.mp3" /INSTALL`
!else
    Exec `"$INSTDIR\wasabi.exe" /INSTALL`
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
      MessageBox MB_RETRYCANCEL "wasabi.player is running already. Please close wasabi.player and hit Retry, or hit Cancel to abort installation." IDRETRY killwinamploop
      Abort
wanotrunning:
!macroend

!ifndef liteinstall
InstType Full
!endif
InstType Lite

SetDateSave on

Section "-wasabi.player.main"
SectionIn RO
!insertmacro CloseStudios

DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "WinampAgent"

#is it needed anymore?
#Delete "$INSTDIR\db\*.*"

SetOutPath $INSTDIR
SetOverwrite on


ClearErrors
GetDLLVersionLocal studio\wasabi.exe $7 $8
IfFileExists $INSTDIR\wasabi.exe "" UpgradeReq
GetDLLVersion $INSTDIR\wasabi.exe $5 $6
IfErrors UpgradeReq
IntCmpU $7 $5 "" DowngradeReq UpgradeReq
IntCmpU $8 $6 "" DowngradeReq UpgradeReq

  ; versions are identical, skip cleanup
  Goto NoUpgrade

  DowngradeReq:        ; downgrading? prompt user 
    StrCmp $2 "" "" NoDownGrade
      MessageBox MB_YESNO "Installed version of wasabi.player is newer than this installer.$\r$\nInstall anyway?" IDYES UpgradeReq
    NoDownGrade:
      Abort "User chooses not to downgrade."
  UpgradeReq: 
    ; at this point the wasabi.exe being installed is newer, or 
    ; there was an error reading the version from $INSTDIR\wasabi.exe
    Delete $INSTDIR\db\*.*
    Delete $INSTDIR\Studio.xnf
    Delete $INSTDIR\Studio.ini
    RMDIR /r "$INSTDIR\avs_presets"

NoUpgrade:

File txt\copyright_notice.txt
File studio\readme.txt
File studio\tips.txt
File Wasabi_Overview.txt
File studio\wasabi.exe
File studio\winampa.exe
File studio\common.dll

WriteIniStr $INSTDIR\wasabi.ini Wasabi MultiUser false

!ifndef liteinstall

File newwhatsnew.txt
File studio\demo.mp3
File studio\mc.exe # this is the maki compiler. Lets give it to everybody (except lite).

!endif

# we set this in case they ever go back to 488 :P
WriteRegStr HKLM "SOFTWARE\Nullsoft\Winamp3\AOD" "AlreadyDropped" "hai"

# winampy essential wacs
SetOutPath $INSTDIR\Wacs
  File /r studio\wacs\winamp

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

#!ifndef liteinstall
#Section "Advanced Visualization Studio"
#SectionIn 1
#SetOutPath $INSTDIR\.
#SetOverwrite off
#File studio.ini  # default avs startup info
#SetOverwrite on
#SetOutPath $INSTDIR\Wacs
#SetOverwrite off
#File vis_avs.dat # default avs preset
#SetOverwrite on
#SetOutPath "$INSTDIR\wacs\data\avs"
#File "studio\wacs\data\avs\avs intro.avs"
#File studio\wacs\data\avs\fyrewurx.ape
#File studio\wacs\data\avs\nullsoft1bit.bmp
#SetOutPath "$INSTDIR\wacs\data\avs\Nullsoft"
#File /r studio\wacs\data\avs\Nullsoft\*.*
#
#SetOutPath $INSTDIR\Wacs\winamp\avs
#File studio\wacs\winamp\avs\avs.wac
#
#SectionEnd # AVS

#!endif

SubSectionEnd # Winamp3

# install latest Wasabi
!define wasabi_player
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

!ifndef liteinstall

!endif

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

WriteIniStr $INSTDIR\wasabi.ini Wasabi MultiUser true

SectionEnd # Per-user settings mode

Section -PostInstall
WriteUninstaller $INSTDIR\uninst-wa3.exe
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\wasabi.player" "DisplayName" "wasabi.player (remove only)"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\wasabi.player" "UninstallString" "$INSTDIR\uninst-wa3.EXE"
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

DeleteRegKey HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\wasabi.player"
Delete "$DESKTOP\wasabi.player.lnk"
Delete "$QUICKLAUNCH\wasabi.player.lnk"
Delete "$SMPROGRAMS\wasabi.player\*.*"
RMDir "$SMPROGRAMS\wasabi.player"

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

#Delete "$INSTDIR\Nullsoft\Winamp\*.*"
#RMDir "$INSTDIR\Nullsoft\Winamp"
#RMDir "$INSTDIR\Nullsoft"

RMDir /r "$INSTDIR\Scripts\Magnifier"
RMDir /r "$INSTDIR\Scripts\DMove"
RMDir "$INSTDIR\Scripts"

Delete "$INSTDIR\AOD\*.*"
RMDir /r "$INSTDIR\AOD"


Delete "$INSTDIR\*.*"
RMDir "$INSTDIR"

RMDir /r "$TEMP\_wa3sktmp"

!include wasabi-un.nsi

RMDir /r "$INSTDIR\Wacs\winamp"

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

AllDone:

SectionEnd # Uninstall

