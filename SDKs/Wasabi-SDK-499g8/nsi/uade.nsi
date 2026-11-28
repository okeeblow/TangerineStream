Name "UADE for Winamp3"
CRCCheck on
ComponentText "This will install UADE for Winamp3 on your computer."
DirText "Setup has determined the optimal location to install. If you would like to change the directory, do so now."
LicenseText "UADE for Winamp3 is released under the GNU GPL license. Please read the license terms below before installing."
LicenseData cnv_uade\uade-license.txt

OutFile install/uade-wa3-1.0.exe
InstallDir "$PROGRAMFILES\Winamp3\Wacs"
AutoCloseWindow false
ShowInstDetails show

Section "UADE for Winamp3"
SetOutPath $INSTDIR
SetOverwrite on
File studio\Wacs\cnv_UADE.wac
SectionEnd

Section -PostInstall
SectionEnd
