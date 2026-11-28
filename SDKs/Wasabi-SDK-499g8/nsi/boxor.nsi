Name "BoxOr skin for Winamp3"
CRCCheck on
ComponentText "This will install the BoxOr skin for Winamp3 on your computer."
DirText "Setup has determined the optimal location to install. If you would like to change the directory, do so now."
#UninstallText "This will uninstall the BoxOr skin for Winamp3. Please hit Next to uninstall, or cancel to keep the BoxOr skin for Winamp3 on your computer."
#UninstallExeName uninst-boxor-wa3.exe
#LicenseText "Winamp3 is a freeware product. Please read the license terms below before installing."
#LicenseData license.txt
OutFile install\boxor-install.exe
InstallDir "$PROGRAMFILES\Winamp3\Skins"
AutoCloseWindow false
ShowInstDetails show

InstType Normal

Section "BoxOr skin"
SetOutPath $INSTDIR
SetOverwrite on
File studio\Skins\BoxOr.wal
SectionEnd

#Section -PostInstall
#WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp3-Boxor" "DisplayName" "BoxOr skin for Winamp3 (remove only)"
#WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp3-Boxor" "UninstallString" "$INSTDIR\uninst-boxor-wa3.EXE"
#SectionEnd

#Section Uninstall
#DeleteRegKey HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp3-Boxor"
#Delete "$INSTDIR\Skins\BoxOr.wal"
#SectionEnd
