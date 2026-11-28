Name "Lounge skin for Winamp3"
CRCCheck on
ComponentText "This will install the Lounge skin for Winamp3 on your computer."
DirText "Setup has determined the optimal location to install. If you would like to change the directory, do so now."
UninstallText "This will uninstall the Lounge skin for Winamp3. Please hit Next to uninstall, or cancel to keep the Lounge skin for Winamp3 on your computer."
UninstallExeName uninst-lounge.exe
#LicenseText "Winamp3 is a freeware product. Please read the license terms below before installing."
#LicenseData license.txt
OutFile install\lounge-install.exe
InstallDir "$PROGRAMFILES\Winamp3\Skins"
AutoCloseWindow false
ShowInstDetails show

InstType Normal

Section "Lounge skin"
SetOutPath $INSTDIR
SetOverwrite on
File studio\Skins\Lounge.wal
SectionEnd

#Section -PostInstall
#WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp3-Lounge" "DisplayName" "Lounge skin for Winamp3 (remove only)"
#WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp3-Lounge" "UninstallString" "$INSTDIR\uninst-lounge.EXE"
#SectionEnd

#Section Uninstall
#DeleteRegKey HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp3-Lounge"
#Delete "$INSTDIR\Skins\Lounge.wal"
#SectionEnd
