# Microsoft Developer Studio Project File - Name="bfc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=bfc - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "bfc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "bfc.mak" CFG="bfc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "bfc - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "bfc - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "$/Studio/bfc"
# PROP Scc_LocalPath ".\bfc"
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "bfc - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "bfc - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
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

# Name "bfc - Win32 Release"
# Name "bfc - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "util"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\bfc\util\pathparse.cpp
# End Source File
# End Group
# Begin Group "wnds"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\bfc\wnds\autofilterlist.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\autoquerylist.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\blankwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\buttbar.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\buttwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\itemlistwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\listwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\treewnd.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\bfc\wnds\abstractwndhold.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\appcmds.cpp
# End Source File
# Begin Source File

SOURCE=.\studio\assert.cpp
# End Source File
# Begin Source File

SOURCE=.\attribs\attribute.cpp
# End Source File
# Begin Source File

SOURCE=.\attribs\attrstr.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\autobitmap.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\backbufferwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\basewnd.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\bitmap.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\blending.cpp
# End Source File
# Begin Source File

SOURCE=.\common\bucketitem.cpp
# End Source File
# Begin Source File

SOURCE=.\common\script\c_script\c_browser.cpp
# End Source File
# Begin Source File

SOURCE=.\common\script\c_script\c_button.cpp
# End Source File
# Begin Source File

SOURCE=.\common\script\c_script\c_group.cpp
# End Source File
# Begin Source File

SOURCE=.\common\script\c_script\c_grouplist.cpp
# End Source File
# Begin Source File

SOURCE=.\common\script\c_script\c_guiobject.cpp
# End Source File
# Begin Source File

SOURCE=.\common\script\c_script\c_rootobj.cpp
# End Source File
# Begin Source File

SOURCE=.\common\script\c_script\c_text.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\canvas.cpp
# End Source File
# Begin Source File

SOURCE=.\attribs\cfgitemi.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\checkbox.cpp
# End Source File
# Begin Source File

SOURCE=.\common\checkwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\common\clickwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\studio\consolecb.cpp
# End Source File
# Begin Source File

SOURCE=.\common\contextcmd.cpp
# End Source File
# Begin Source File

SOURCE=.\common\contextmenu.cpp
# End Source File
# Begin Source File

SOURCE=.\common\contwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\studio\corecb.cpp
# End Source File
# Begin Source File

SOURCE=.\common\corehandle.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\depend.cpp
# End Source File
# Begin Source File

SOURCE=.\common\dragitemi.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\dropdownlist.cpp
# End Source File
# Begin Source File

SOURCE=.\common\droplist.cpp
# End Source File
# Begin Source File

SOURCE=.\common\editwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\encodedstr.cpp
# End Source File
# Begin Source File

SOURCE=.\common\filename.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\filteredcolor.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\findobjectcb.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\foreach.cpp
# End Source File
# Begin Source File

SOURCE=.\common\formwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\common\framewnd.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\freelist.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\groupclickwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\grouptgbutton.cpp
# End Source File
# Begin Source File

SOURCE=.\common\guiobjwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\guistatuscb.cpp
# End Source File
# Begin Source File

SOURCE=.\common\script\c_script\h_browser.cpp
# End Source File
# Begin Source File

SOURCE=.\common\script\c_script\h_button.cpp
# End Source File
# Begin Source File

SOURCE=.\common\script\c_script\h_group.cpp
# End Source File
# Begin Source File

SOURCE=.\common\script\c_script\h_guiobject.cpp
# End Source File
# Begin Source File

SOURCE=.\common\script\c_script\h_rootobj.cpp
# End Source File
# Begin Source File

SOURCE=.\common\script\c_script\h_text.cpp
# End Source File
# Begin Source File

SOURCE=.\studio\xml\hashtable.c
# End Source File
# Begin Source File

SOURCE=.\common\intarray.cpp
# End Source File
# Begin Source File

SOURCE=.\common\labelwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\common\mainminibrowser.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\memblock.cpp
# End Source File
# Begin Source File

SOURCE=.\common\metadatasvc.cpp
# End Source File
# Begin Source File

SOURCE=.\common\minibrowser.cpp
# End Source File
# Begin Source File

SOURCE=.\common\minibrowserwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\db\multiqueryclient.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\db\multiqueryserver.cpp
# End Source File
# Begin Source File

SOURCE=.\common\nsGUID.cpp
# End Source File
# Begin Source File

SOURCE=.\common\script\objcontroller.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\paintcb.cpp
# End Source File
# Begin Source File

SOURCE=.\common\playstring.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\popexitcb.cpp
# End Source File
# Begin Source File

SOURCE=.\common\popup.cpp
# End Source File
# Begin Source File

SOURCE=.\Common\profiler.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\ptrlist.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\qpaintwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\common\radiogroup.cpp
# End Source File
# Begin Source File

SOURCE=.\common\radiownd.cpp
# End Source File
# Begin Source File

SOURCE=.\common\readdir.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\region.cpp
# End Source File
# Begin Source File

SOURCE=.\common\rootcomp.cpp
# End Source File
# Begin Source File

SOURCE=.\common\script\rootobj.cpp
# End Source File
# Begin Source File

SOURCE=.\common\script\rootobjcb.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\rootwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\rootwndholder.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\db\scannerserver.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\scbkgwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\common\script\c_script\scripthook.cpp
# End Source File
# Begin Source File

SOURCE=.\common\script\scriptobj.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\scrollbar.cpp
# End Source File
# Begin Source File

SOURCE=.\common\selectfile.cpp
# End Source File
# Begin Source File

SOURCE=.\common\sepwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\studio\sequence.cpp
# End Source File
# Begin Source File

SOURCE=.\studio\services\servicei.cpp
# End Source File
# Begin Source File

SOURCE=.\common\servicewnd.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\db\sharedscanner.cpp
# End Source File
# Begin Source File

SOURCE=.\studio\xml\simap.cpp
# End Source File
# Begin Source File

SOURCE=.\studio\skincb.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\skinclr.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\skinwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\common\slider.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\stack.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\status.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\std.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\string.cpp
# End Source File
# Begin Source File

SOURCE=.\common\styleposwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\db\subqueryserver.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\svc_enum.cpp
# End Source File
# Begin Source File

SOURCE=.\studio\services\svc_imgload.cpp
# End Source File
# Begin Source File

SOURCE=.\studio\services\svc_wndcreate.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\svcwndhold.cpp
# End Source File
# Begin Source File

SOURCE=.\studio\syscb.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\tabsheet.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\tabsheetbar.cpp
# End Source File
# Begin Source File

SOURCE=.\common\textbar.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\timerclient.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\timermul.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\timeslicer.cpp
# End Source File
# Begin Source File

SOURCE=.\common\tipwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\tooltip.cpp
# End Source File
# Begin Source File

SOURCE=.\common\transcendentals.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\wnds\typesheet.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\u8string.cpp
# End Source File
# Begin Source File

SOURCE=.\common\urlgrab.cpp
# End Source File
# Begin Source File

SOURCE=.\bfc\virtualwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\common\wildcharsenum.cpp
# End Source File
# Begin Source File

SOURCE=.\common\wndcb.cpp
# End Source File
# Begin Source File

SOURCE=.\studio\xmlmanagedobj.cpp
# End Source File
# Begin Source File

SOURCE=.\common\xmlobject.cpp
# End Source File
# Begin Source File

SOURCE=.\studio\xmlparse.cpp
# End Source File
# Begin Source File

SOURCE=.\studio\xml\xmlparser.c
# End Source File
# Begin Source File

SOURCE=.\studio\xmlreader.cpp
# End Source File
# Begin Source File

SOURCE=.\studio\xml\xmlrole.c
# End Source File
# Begin Source File

SOURCE=.\studio\xml\xmltok.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\common\script\c_script\c_grouplist.h
# End Source File
# Begin Source File

SOURCE=.\bfc\popexitcb.h
# End Source File
# Begin Source File

SOURCE=.\common\readdir.h
# End Source File
# Begin Source File

SOURCE=.\common\selectfile.h
# End Source File
# End Group
# End Target
# End Project
