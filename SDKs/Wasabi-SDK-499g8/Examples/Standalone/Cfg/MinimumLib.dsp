# Microsoft Developer Studio Project File - Name="MinimumLib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=MinimumLib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MinimumLib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MinimumLib.mak" CFG="MinimumLib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MinimumLib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "MinimumLib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MinimumLib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "../../" /I "Cfg/Minimum" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Yu"precomp.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "MinimumLib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../../" /I "Cfg/Minimum" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "USE_MEM_CHECK" /Yu"precomp.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "MinimumLib - Win32 Release"
# Name "MinimumLib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "system"

# PROP Default_Filter ""
# Begin Group "file"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\bfc\parse\pathparse.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\common\wildcharsenum.cpp
# End Source File
# End Group
# Begin Group "memory"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\\bfc\foreach.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\bfc\memblock.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\bfc\ptrlist.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\bfc\stack.cpp
# End Source File
# End Group
# Begin Group "string"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\\bfc\encodedstr.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\bfc\string.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\\studio\assert.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\common\critsec.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\bfc\depend.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\common\nsGUID.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\bfc\std.cpp
# End Source File
# End Group
# Begin Group "apis"

# PROP Default_Filter ""
# Begin Group "svc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\\bfc\api\api_service.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\bfc\api\api_servicei.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\bfc\api\api_servicex.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\studio\consolecb.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\bfc\svc_enum.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\bfc\svcenum.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\bfc\svcenumbyguid.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\bfc\svcenumt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\studio\svcmgr.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\Studio\services\waservicefactory.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\Studio\services\waservicefactorybase.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\Studio\services\waservicefactoryi.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\Studio\services\waservicefactoryt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\Studio\services\waservicefactoryx.cpp
# End Source File
# End Group
# Begin Group "app"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\\bfc\api\api_application.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\bfc\api\api_applicationi.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\bfc\api\api_applicationx.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\bfc\util\inifile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\studio\pathmgr.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\studio\version.cpp
# End Source File
# End Group
# Begin Group "syscb"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\\bfc\api\api_syscb.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\bfc\api\api_syscbi.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\bfc\api\api_syscbx.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\studio\cbmgr.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\studio\syscb.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\Studio\syscbi.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\Studio\syscbx.cpp
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=..\..\\wasabi\api\apiinit.cpp
# End Source File
# Begin Source File

SOURCE=..\..\\precomp.cpp
# ADD CPP /Yc"precomp.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
