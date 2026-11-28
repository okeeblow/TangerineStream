# Microsoft Developer Studio Project File - Name="cnv_vorbis" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=cnv_vorbis - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cnv_vorbis.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cnv_vorbis.mak" CFG="cnv_vorbis - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cnv_vorbis - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cnv_vorbis - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "$/Studio/cnv_vorbis"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cnv_vorbis - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "cnv_vorbis_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /GX /Zi /O2 /I "vorbis" /I ".\\" /I "../studio/wa3cfg" /I "../" /D "NDEBUG" /D "cnv_rawpcm_EXPORTS" /D "WACLIENT_NOICONSUPPORT" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "USE_ASM" /D "COMMON_STATIC_LINK" /Yu"precomp.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib msacm32.lib winmm.lib /nologo /dll /pdb:none /debug /machine:I386 /out:"../studio/Wacs/wasabi.player/mediacore/cnv/vorbis/cnv_vorbis.wac"

!ELSEIF  "$(CFG)" == "cnv_vorbis - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "cnv_vorbis_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /I "vorbis" /I ".\\" /I "../studio/wa3cfg" /I "../" /D "_DEBUG" /D "REAL_STDIO" /D "cnv_vorbis_EXPORTS" /D "WACLIENT_NOICONSUPPORT" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "USE_ASM" /D "COMMON_STATIC_LINK" /Yu"precomp.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib msacm32.lib winmm.lib /nologo /dll /debug /machine:I386 /out:"../studio/Wacs/wasabi.player/mediacore/cnv/vorbis/cnv_vorbis.wac" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "cnv_vorbis - Win32 Release"
# Name "cnv_vorbis - Win32 Debug"
# Begin Group "wac"

# PROP Default_Filter ""
# End Group
# Begin Group "libvorbis"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\vorbis\analysis.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\backends.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\barkmel.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\bitbuffer.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\bitbuffer.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\bitrate.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\bitwise.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\block.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\codebook.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\codebook.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\codec.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\codec_internal.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\envelope.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\envelope.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\floor0.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\floor1.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\framing.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\iir.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\info.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\lookup.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\lookup.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\lookup_data.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\lpc.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\lpc.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\lsp.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\lsp.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\mapping0.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\masking.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\mdct.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\mdct.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\misc.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\os.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\psy.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\psy.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\registry.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\registry.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\res0.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\scales.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\sharedbook.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\smallft.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\smallft.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\synthesis.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\vorbisenc.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\vorbisfile.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\vorbisfile.h
# End Source File
# Begin Source File

SOURCE=.\vorbis\window.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vorbis\window.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\cnv_vorbis.cpp
# End Source File
# Begin Source File

SOURCE=.\cnv_vorbis.h
# End Source File
# Begin Source File

SOURCE=.\newhack.h
# End Source File
# Begin Source File

SOURCE=..\precomp.cpp
# ADD CPP /Yc"precomp.h"
# End Source File
# Begin Source File

SOURCE=.\setinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\vorbis.cpp
# End Source File
# Begin Source File

SOURCE=.\vorbis.h
# End Source File
# Begin Source File

SOURCE=.\vorbis_nsv.cpp
# End Source File
# Begin Source File

SOURCE=.\vorbis_nsv.h
# End Source File
# End Target
# End Project
