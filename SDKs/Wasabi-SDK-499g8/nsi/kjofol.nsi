Name "Kjofol skin for Winamp3"
CRCCheck on
ComponentText "This will install the Kjofol skin for Winamp3 on your computer."
DirText "Setup has determined the optimal location to install. If you would like to change the directory, do so now."
UninstallText "This will uninstall the Kjofol skin for Winamp3. Please hit Next to uninstall, or cancel to keep the Kjofol skin for Winamp3 on your computer."
UninstallExeName uninst-Kjofol.exe
#LicenseText "Winamp3 is a freeware product. Please read the license terms below before installing."
#LicenseData license.txt
OutFile install\Kjofol-install.exe
InstallDir "$PROGRAMFILES\Winamp3\Skins"
AutoCloseWindow false
ShowInstDetails show

InstType Normal

Section "Kjofol skin"
SetOutPath $INSTDIR
SetOverwrite on
File studio\Skins\Kjofol.wal
SectionEnd

#Section -PostInstall
#WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp3-Kjofol" "DisplayName" "Kjofol skin for Winamp3 (remove only)"
#WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp3-Kjofol" "UninstallString" "$INSTDIR\uninst-Kjofol.EXE"
#SectionEnd

#Section Uninstall
#DeleteRegKey HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp3-Kjofol"
#Delete "$INSTDIR\Skins\Kjofol.wal"
#SectionEnd
