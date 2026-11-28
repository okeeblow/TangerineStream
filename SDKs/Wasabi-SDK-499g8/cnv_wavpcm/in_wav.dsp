# Microsoft Developer Studio Project File - Name="in_wav" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=in_wav - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "in_wav.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "in_wav.mak" CFG="in_wav - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "in_wav - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "in_wav - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "in_wav - Win32 Llama Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "in_wav - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "PP_NO_ARG" /D "PP_NO_CONSOLE" /D "WA2" /D "USE_PP_CRT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x415 /d "NDEBUG"
# ADD RSC /l 0x415 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib msacm32.lib pp_crt.lib /nologo /entry:"DllMain" /subsystem:windows /dll /machine:I386 /nodefaultlib /out:"c:\program files\winamp\plugins\in_wave.dll"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "in_wav - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "PP_NO_ARG" /D "PP_NO_CONSOLE" /D "WA2" /D "HUNT_LEAKS" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x415 /d "_DEBUG"
# ADD RSC /l 0x415 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib msacm32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"c:\program files\winamp\plugins\in_wave.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "in_wav - Win32 Llama Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "in_wav___Win32_Llama_Release"
# PROP BASE Intermediate_Dir "in_wav___Win32_Llama_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "in_wav___Win32_Llama_Release"
# PROP Intermediate_Dir "in_wav___Win32_Llama_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "PP_NO_ARG" /D "PP_NO_CONSOLE" /D "WA2" /D "USE_PP_CRT" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "PP_NO_ARG" /D "PP_NO_CONSOLE" /D "WA2" /D "USE_PP_CRT" /D "LLAMA" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x415 /d "NDEBUG"
# ADD RSC /l 0x415 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib msacm32.lib pp_crt.lib /nologo /entry:"DllMain" /subsystem:windows /dll /machine:I386 /nodefaultlib /out:"c:\program files\winamp\plugins\in_wave.dll"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib msacm32.lib pp_crt.lib /nologo /entry:"DllMain" /subsystem:windows /dll /machine:I386 /nodefaultlib /out:"c:\program files\winamp\plugins\in_wave.dll"
# SUBTRACT LINK32 /debug

!ENDIF 

# Begin Target

# Name "in_wav - Win32 Release"
# Name "in_wav - Win32 Debug"
# Name "in_wav - Win32 Llama Release"
# Begin Source File

SOURCE=.\adpcm.cpp
# End Source File
# Begin Source File

SOURCE=.\adpcm.h
# End Source File
# Begin Source File

SOURCE=.\aiff.cpp
# End Source File
# Begin Source File

SOURCE=.\au.cpp
# End Source File
# Begin Source File

SOURCE=.\aud.cpp
# End Source File
# Begin Source File

SOURCE=.\aud.h
# End Source File
# Begin Source File

SOURCE=.\chunk.h
# End Source File
# Begin Source File

SOURCE=.\config.cpp
# End Source File
# Begin Source File

SOURCE=.\core_api.h
# End Source File
# Begin Source File

SOURCE=.\g711.cpp

!IF  "$(CFG)" == "in_wav - Win32 Release"

!ELSEIF  "$(CFG)" == "in_wav - Win32 Debug"

!ELSEIF  "$(CFG)" == "in_wav - Win32 Llama Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g72x.cpp

!IF  "$(CFG)" == "in_wav - Win32 Release"

# ADD CPP /O2

!ELSEIF  "$(CFG)" == "in_wav - Win32 Debug"

!ELSEIF  "$(CFG)" == "in_wav - Win32 Llama Release"

# ADD BASE CPP /O2
# ADD CPP /O2

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ima.cpp
# End Source File
# Begin Source File

SOURCE=.\In2.h
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\main.h
# End Source File
# Begin Source File

SOURCE=.\misc.cpp
# End Source File
# Begin Source File

SOURCE=.\Out.h
# End Source File
# Begin Source File

SOURCE=.\pp_crt.h
# End Source File
# Begin Source File

SOURCE=.\voc.cpp
# End Source File
# Begin Source File

SOURCE=.\wa2.cpp
# End Source File
# Begin Source File

SOURCE=.\wav.cpp
# End Source File
# Begin Source File

SOURCE=.\wav.rc

!IF  "$(CFG)" == "in_wav - Win32 Release"

!ELSEIF  "$(CFG)" == "in_wav - Win32 Debug"

!ELSEIF  "$(CFG)" == "in_wav - Win32 Llama Release"

# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409 /d "LLAMA"

!ENDIF 

# End Source File
# End Target
# End Project
