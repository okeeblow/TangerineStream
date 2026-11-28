Name "Simple skin for Winamp3"
CRCCheck on
ComponentText "This will install the Simple skin for Winamp3 on your computer."
DirText "Setup has determined the optimal location to install. If you would like to change the directory, do so now."
OutFile install\simple-install.exe
InstallDir "$PROGRAMFILES\Winamp3\Skins"
AutoCloseWindow false
ShowInstDetails show

InstType Normal
InstType Full

Section "Simple skin"
SetOutPath $INSTDIR
SetOverwrite on
File studio\Skins\Simple.wal
SectionEnd

Section "base.psd file (for skinners)"
SectionIn 2
SetOutPath $INSTDIR
SetOverwrite on
File studio\Skins\Simple\gfx\base.psd
SectionEnd

Section -PostInstall
SectionEnd
