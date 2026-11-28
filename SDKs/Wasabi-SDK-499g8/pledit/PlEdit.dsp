# Microsoft Developer Studio Project File - Name="PlEdit" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=PlEdit - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "pledit.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "pledit.mak" CFG="PlEdit - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PlEdit - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "PlEdit - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "$/Studio/PlEdit"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "PlEdit - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PLEDIT_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "../studio/wa3cfg" /I "../" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PLEDIT_EXPORTS" /D "COMMON_STATIC_LINK" /Yu"precomp.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:none /debug /machine:I386 /out:"../studio/Wacs/wasabi.player/pledit/pledit.wac"

!ELSEIF  "$(CFG)" == "PlEdit - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PLEDIT_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../studio/wa3cfg" /I "../" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PLEDIT_EXPORTS" /D "COMMON_STATIC_LINK" /Yu"precomp.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"../studio/Wacs/wasabi.player/pledit/pledit.wac" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "PlEdit - Win32 Release"
# Name "PlEdit - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\asx.cpp
# End Source File
# Begin Source File

SOURCE=.\bpl.cpp
# End Source File
# Begin Source File

SOURCE=.\csv.cpp
# End Source File
# Begin Source File

SOURCE=.\defdrop.cpp
# End Source File
# Begin Source File

SOURCE=.\dirload.cpp
# End Source File
# Begin Source File

SOURCE=.\editor.cpp
# End Source File
# Begin Source File

SOURCE=.\m3u.cpp
# End Source File
# Begin Source File

SOURCE=.\playlist.cpp
# End Source File
# Begin Source File

SOURCE=.\plcmd.cpp
# End Source File
# Begin Source File

SOURCE=.\plcmdline.cpp
# End Source File
# Begin Source File

SOURCE=.\pldir.cpp
# End Source File
# Begin Source File

SOURCE=.\pldirobj.cpp
# End Source File
# Begin Source File

SOURCE=.\pldirsvc.cpp
# End Source File
# Begin Source File

SOURCE=.\pldirwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\pledit.cpp
# End Source File
# Begin Source File

SOURCE=.\plhtml.cpp
# End Source File
# Begin Source File

SOURCE=.\plitem2.cpp
# End Source File
# Begin Source File

SOURCE=.\plload.cpp
# End Source File
# Begin Source File

SOURCE=.\pls.cpp
# End Source File
# Begin Source File

SOURCE=.\plseq.cpp
# End Source File
# Begin Source File

SOURCE=.\plslot.cpp
# End Source File
# Begin Source File

SOURCE=.\plstatus.cpp
# End Source File
# Begin Source File

SOURCE=..\precomp.cpp
# ADD CPP /Yc"precomp.h"
# End Source File
# Begin Source File

SOURCE=.\queryload.cpp
# End Source File
# Begin Source File

SOURCE=.\selwnd.cpp
# End Source File
# Begin Source File

SOURCE=..\core\sequence.cpp
# End Source File
# Begin Source File

SOURCE=..\studio\skincb.cpp
# End Source File
# Begin Source File

SOURCE=.\svc_playlist.cpp
# End Source File
# Begin Source File

SOURCE=.\svc_pldir.cpp
# End Source File
# Begin Source File

SOURCE=..\bfc\util\timefmt.cpp
# End Source File
# Begin Source File

SOURCE=.\wndcreate.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\playlist.h
# End Source File
# Begin Source File

SOURCE=.\plcmdline.h
# End Source File
# Begin Source File

SOURCE=.\pledit.h
# End Source File
# Begin Source File

SOURCE=.\plitem.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
