# Microsoft Developer Studio Project File - Name="guiobjects" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=guiobjects - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "guiobjects.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "guiobjects.mak" CFG="guiobjects - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "guiobjects - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "guiobjects - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "$/Studio/guiobjects"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "guiobjects - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "guiobjects_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "../studio/wa3cfg" /I "../" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "guiobjects_EXPORTS" /D "COMMON_STATIC_LINK" /Yu"precomp.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:none /debug /machine:I386 /out:"../studio/Wacs/wasabi.system/ui/guiobjects/guiobjects.wac"

!ELSEIF  "$(CFG)" == "guiobjects - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "guiobjects_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../studio/wa3cfg" /I "../" /D "_DEBUG" /D "WACLIENT_NOICONSUPPORT" /D "REAL_STDIO" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "guiobjects_EXPORTS" /D "COMMON_STATIC_LINK" /Yu"precomp.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"../studio/Wacs/wasabi.system/ui/guiobjects/guiobjects.wac" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "guiobjects - Win32 Release"
# Name "guiobjects - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "xuiobjects"

# PROP Default_Filter ""
# Begin Group "xpp"

# PROP Default_Filter ""
# Begin Group "generated"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\xuimenuso.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\guiobjects.xpp

!IF  "$(CFG)" == "guiobjects - Win32 Release"

# Begin Custom Build
InputDir=.
InputPath=.\guiobjects.xpp
InputName=guiobjects

BuildCmds= \
	..\bin\xpreproc /i:$(InputDir)\$(InputName).xpp /y

"$(InputDir)\xuimenuso.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\xuimenuso.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\xuimenu.mi" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "guiobjects - Win32 Debug"

# Begin Custom Build
InputDir=.
InputPath=.\guiobjects.xpp
InputName=guiobjects

BuildCmds= \
	..\bin\xpreproc /i:$(InputDir)\$(InputName).xpp /y

"$(InputDir)\xuimenuso.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\xuimenuso.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\xuimenu.mi" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\xuiaddparams.cpp
# End Source File
# Begin Source File

SOURCE=.\xuibrowser.cpp
# End Source File
# Begin Source File

SOURCE=.\xuicheckbox.cpp
# End Source File
# Begin Source File

SOURCE=.\xuicombobox.cpp
# End Source File
# Begin Source File

SOURCE=.\xuicustomobject.cpp
# End Source File
# Begin Source File

SOURCE=.\xuidropdownlist.cpp
# End Source File
# Begin Source File

SOURCE=.\xuifilterlist.cpp
# End Source File
# Begin Source File

SOURCE=.\xuiformwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\xuiformwndso.cpp
# End Source File
# Begin Source File

SOURCE=.\xuiframe.cpp
# End Source File
# Begin Source File

SOURCE=.\xuigradientwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\xuigrid.cpp
# End Source File
# Begin Source File

SOURCE=.\xuihideobject.cpp
# End Source File
# Begin Source File

SOURCE=.\xuihistoryedit.cpp
# End Source File
# Begin Source File

SOURCE=.\xuilist.cpp
# End Source File
# Begin Source File

SOURCE=.\xuimenu.cpp
# End Source File
# Begin Source File

SOURCE=.\xuiobjdirwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\xuipathpicker.cpp
# End Source File
# Begin Source File

SOURCE=.\xuiquerydrag.cpp
# End Source File
# Begin Source File

SOURCE=.\xuiqueryline.cpp
# End Source File
# Begin Source File

SOURCE=.\xuiquerylist.cpp
# End Source File
# Begin Source File

SOURCE=.\xuiradiogroup.cpp
# End Source File
# Begin Source File

SOURCE=.\xuirect.cpp
# End Source File
# Begin Source File

SOURCE=.\xuisendparams.cpp
# End Source File
# Begin Source File

SOURCE=.\xuistatus.cpp
# End Source File
# Begin Source File

SOURCE=.\xuitabsheet.cpp
# End Source File
# Begin Source File

SOURCE=.\xuititlebox.cpp
# End Source File
# Begin Source File

SOURCE=.\xuitree.cpp
# End Source File
# Begin Source File

SOURCE=.\xuiwndholder.cpp
# End Source File
# End Group
# Begin Group "bfc.db"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\bfc\db\multiqueryclient.cpp
# End Source File
# Begin Source File

SOURCE=..\bfc\db\multiqueryserver.cpp
# End Source File
# Begin Source File

SOURCE=..\bfc\db\scannerserver.cpp
# End Source File
# Begin Source File

SOURCE=..\bfc\db\sharedscanner.cpp
# End Source File
# Begin Source File

SOURCE=..\bfc\db\subqueryserver.cpp
# End Source File
# End Group
# Begin Group "bfc.wnds"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\bfc\wnds\autofilterlist.cpp
# End Source File
# Begin Source File

SOURCE=..\bfc\wnds\autoquerylist.cpp
# End Source File
# Begin Source File

SOURCE=..\bfc\wnds\gradientwnd.cpp
# End Source File
# Begin Source File

SOURCE=..\bfc\wnds\objdirwnd.cpp
# End Source File
# Begin Source File

SOURCE=..\bfc\wnds\queryline.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\feed_svcs.cpp
# End Source File
# Begin Source File

SOURCE=.\grouptips.cpp
# End Source File
# Begin Source File

SOURCE=.\guiobjects.cpp
# End Source File
# Begin Source File

SOURCE=..\bfc\guistatuscb.cpp
# End Source File
# Begin Source File

SOURCE=.\multelementsvc.cpp
# End Source File
# Begin Source File

SOURCE=..\precomp.cpp
# ADD CPP /Yc"precomp.h"
# End Source File
# Begin Source File

SOURCE=.\svc.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\db.h
# End Source File
# Begin Source File

SOURCE=.\loader.h
# End Source File
# Begin Source File

SOURCE=.\xuicheckbox.h
# End Source File
# Begin Source File

SOURCE=.\xuiradiogroup.h
# End Source File
# Begin Source File

SOURCE=.\xuitree.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
