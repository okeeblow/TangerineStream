#
#	IF YOU ADD FOLDERS TO THE SDK, MAKE SURE YOU ADD THEM TO THE UNINSTALLER.
#


Name "Wasabi SDK"
CRCCheck on
ComponentText "blah"
DirText "Please choose where you would like to install the SDK."
UninstallText "This will uninstall the Wasabi SDK. Please hit Next to uninstall, or cancel to keep the SDK on your computer."
LicenseText "Please read the license terms below before installing. Bitch."
LicenseData sdk\sdklicense.txt
Outfile install\wasabi-sdk.exe
InstallDir "C:\Wasabi SDK"
AutoCloseWindow false
ShowInstDetails show

SetDateSave on

Section "Wasabi SDK"
SectionIn RO
SetOutPath $INSTDIR
SetOverwrite on
File sdk\*.txt
File project2make.pl

SetOutPath $INSTDIR
File sdk\*.dsp
File precomp.h
File precomp.cpp

SetOutPath $INSTDIR\nsi
File *.nsi

SetOutPath $INSTDIR\studio
File sdk\studio\mc.exe

SetOutPath $INSTDIR\studio\script
File sdk\studio\script\*.*

SetOutPath $INSTDIR\studio\lib
File sdk\studio\lib\*.m
File sdk\studio\lib\*.mi

SetOutPath $INSTDIR\bin
File sdk\bin\*.exe

SetOutPath $INSTDIR\bin\templates
File sdk\bin\templates\*.*

SetOutPath $INSTDIR\studio\templates\data
File sdk\bin\templates\data\*.*

SetOutPath $INSTDIR\docs
File sdk\docs\*.txt
File txt\codingstyle.txt

SetOutPath $INSTDIR\bfc
File sdk\bfc\*.*

SetOutPath $INSTDIR\bfc\api
File sdk\bfc\api\*.*

SetOutPath $INSTDIR\bfc\attribs
File sdk\bfc\attribs\*.*

SetOutPath $INSTDIR\bfc\draw
File sdk\bfc\draw\*.*

SetOutPath $INSTDIR\bfc\feeds
File sdk\bfc\feeds\*.*

SetOutPath $INSTDIR\bfc\http
File sdk\bfc\http\*.*

SetOutPath $INSTDIR\bfc\parse
File sdk\bfc\parse\*.*

SetOutPath $INSTDIR\bfc\platform
File sdk\bfc\platform\*.*

SetOutPath $INSTDIR\bfc\platform\win32
File sdk\bfc\platform\win32\*.*

SetOutPath $INSTDIR\bfc\platform\linux
File sdk\bfc\platform\linux\*.*

SetOutPath $INSTDIR\bfc\platform\freetype
File sdk\bfc\platform\freetype\*.*

SetOutPath $INSTDIR\bfc\wnds
File sdk\bfc\wnds\*.*

SetOutPath $INSTDIR\bfc\wnds\form
File sdk\bfc\wnds\form\*.*

SetOutPath $INSTDIR\bfc\db
File sdk\bfc\db\*.*

SetOutPath $INSTDIR\bfc\util
File sdk\bfc\util\*.*

SetOutPath $INSTDIR\bfc\xml
File sdk\bfc\xml\*.*

SetOutPath $INSTDIR\bfc\cdda
File sdk\bfc\cdda\*.*

SetOutPath $INSTDIR\common
File sdk\common\*.h
File sdk\common\*.cpp

SetOutPath $INSTDIR\common\script
File sdk\common\script\*.*

SetOutPath $INSTDIR\common\script\c_script
File sdk\common\script\c_script\*.*

SetOutPath $INSTDIR\core
File sdk\core\*.h
File sdk\core\*.cpp

SetOutPath $INSTDIR\jnetlib
File sdk\jnetlib\*.h
File sdk\jnetlib\*.cpp
File sdk\jnetlib\*.dsp

SetOutPath $INSTDIR\studio
File sdk\studio\*.h
File sdk\studio\*.cpp

SetOutPath $INSTDIR\taskband
File sdk\taskband\*.h
File sdk\taskband\*.cpp
File sdk\taskband\*.dsp
File sdk\taskband\*.rc

SetOutPath $INSTDIR\studio\services
File sdk\studio\services\*.h
File sdk\studio\services\*.cpp

SetOutPath $INSTDIR\studio\xml
File sdk\studio\xml\*.*

SetOutPath $INSTDIR\studio\wa3cfg
File sdk\Studio\wa3cfg\*.*

SetOutPath $INSTDIR\studio\wa5cfg
File sdk\Studio\wa5cfg\*.*

SetOutPath $INSTDIR\waclient
File sdk\waclient\*.dsp

SetOutPath $INSTDIR\wasabi
File sdk\wasabi\*.cpp
File sdk\wasabi\*.h

SetOutPath $INSTDIR\wasabi\api
File sdk\wasabi\api\*.cpp
File sdk\wasabi\api\*.h

SetOutPath $INSTDIR\studio\Locales
File sdk\studio\Locales\francais.xml
File sdk\studio\Locales\quebecois.xml

WriteUninstaller uninst-wsdk.exe

SectionEnd

Section "Example components"

#CUTSetOutPath $INSTDIR\studio

SetOutPath $INSTDIR\Examples
File sdk\Examples\*.dsw
File sdk\Examples\*.dsp
File sdk\Examples\*.cpp
File sdk\Examples\START_HERE.txt

SetOutPath $INSTDIR\Examples\Example1
File sdk\Examples\Example1\*.h
File sdk\Examples\Example1\*.cpp
File sdk\Examples\Example1\*.dsp
File sdk\Examples\Example1\*.png
File sdk\Examples\Example1\*.rc

SetOutPath $INSTDIR\Examples\ExampleAttrib
File sdk\Examples\ExampleAttrib\*.h
File sdk\Examples\ExampleAttrib\*.cpp
File sdk\Examples\ExampleAttrib\*.dsp
File sdk\Examples\ExampleAttrib\*.png
File sdk\Examples\ExampleAttrib\*.rc

SetOutPath $INSTDIR\Examples\ExampleCanvas
File sdk\Examples\ExampleCanvas\*.h
File sdk\Examples\ExampleCanvas\*.cpp
File sdk\Examples\ExampleCanvas\*.dsp
File sdk\Examples\ExampleCanvas\*.png
File sdk\Examples\ExampleCanvas\*.rc

SetOutPath $INSTDIR\Examples\ExampleCodeGUI
File sdk\Examples\examplecodegui\*.h
File sdk\Examples\examplecodegui\*.cpp
File sdk\Examples\examplecodegui\*.dsp
File sdk\Examples\examplecodegui\*.png
File sdk\Examples\examplecodegui\*.rc

SetOutPath $INSTDIR\Examples\ExampleCoreCallback
File sdk\Examples\ExampleCoreCallback\*.h
File sdk\Examples\ExampleCoreCallback\*.cpp
File sdk\Examples\ExampleCoreCallback\*.dsp
File sdk\Examples\ExampleCoreCallback\*.png
File sdk\Examples\ExampleCoreCallback\*.rc

SetOutPath $INSTDIR\Examples\ExampleDragDrop
File sdk\Examples\ExampleDragDrop\*.h
File sdk\Examples\ExampleDragDrop\*.cpp
File sdk\Examples\ExampleDragDrop\*.dsp
File sdk\Examples\ExampleDragDrop\*.dsw

SetOutPath $INSTDIR\Examples\ExampleDsp
File sdk\Examples\ExampleDsp\*.h
File sdk\Examples\ExampleDsp\*.cpp
File sdk\Examples\ExampleDsp\*.dsp
File sdk\Examples\ExampleDsp\*.dsw

SetOutPath $INSTDIR\Examples\ExampleExternElement
File sdk\Examples\ExampleExternElement\*.h
File sdk\Examples\ExampleExternElement\*.cpp
File sdk\Examples\ExampleExternElement\*.dsp
File sdk\Examples\ExampleExternElement\*.png
File sdk\Examples\ExampleExternElement\*.rc

SetOutPath $INSTDIR\Examples\ExampleFreeFormWac
File sdk\Examples\ExampleFreeFormWac\*.h
File sdk\Examples\ExampleFreeFormWac\*.cpp
File sdk\Examples\ExampleFreeFormWac\*.dsp
File sdk\Examples\ExampleFreeFormWac\*.png
File sdk\Examples\ExampleFreeFormWac\*.rc

SetOutPath $INSTDIR\Examples\ExampleHButton
File sdk\Examples\ExampleHButton\*.h
File sdk\Examples\ExampleHButton\*.cpp
File sdk\Examples\ExampleHButton\*.dsp
File sdk\Examples\ExampleHButton\*.dsw

SetOutPath $INSTDIR\Examples\ExampleHTTPXML
File sdk\Examples\ExampleHTTPXML\*.h
File sdk\Examples\ExampleHTTPXML\*.cpp
File sdk\Examples\ExampleHTTPXML\*.dsp
File sdk\Examples\ExampleHTTPXML\*.png
File sdk\Examples\ExampleHTTPXML\*.rc

SetOutPath $INSTDIR\Examples\ExampleItemSequencer
File sdk\Examples\ExampleItemSequencer\*.h
File sdk\Examples\ExampleItemSequencer\*.cpp
File sdk\Examples\ExampleItemSequencer\*.dsp
File sdk\Examples\ExampleItemSequencer\*.png
File sdk\Examples\ExampleItemSequencer\*.rc

SetOutPath $INSTDIR\Examples\ExampleMetaData
File sdk\Examples\ExampleMetaData\*.h
File sdk\Examples\ExampleMetaData\*.cpp
File sdk\Examples\ExampleMetaData\*.dsp

SetOutPath $INSTDIR\Examples\ExampleMW
File sdk\Examples\ExampleMW\*.h
File sdk\Examples\ExampleMW\*.cpp
File sdk\Examples\ExampleMW\*.dsp
File sdk\Examples\ExampleMW\*.png
File sdk\Examples\ExampleMW\*.rc

SetOutPath $INSTDIR\Examples\ExampleOSWnd
File sdk\Examples\ExampleOSWnd\*.h
File sdk\Examples\ExampleOSWnd\*.cpp
File sdk\Examples\ExampleOSWnd\*.dsp
File sdk\Examples\ExampleOSWnd\*.png
File sdk\Examples\ExampleOSWnd\*.rc

SetOutPath $INSTDIR\Examples\ExamplePaintCB
File sdk\Examples\ExamplePaintCB\*.h
File sdk\Examples\ExamplePaintCB\*.cpp
File sdk\Examples\ExamplePaintCB\*.dsp
File sdk\Examples\ExamplePaintCB\*.rc

SetOutPath $INSTDIR\Examples\ExamplePl
File sdk\Examples\ExamplePl\*.h
File sdk\Examples\ExamplePl\*.cpp
File sdk\Examples\ExamplePl\*.dsp

SetOutPath $INSTDIR\Examples\ExamplePopup
File sdk\Examples\ExamplePopup\*.h
File sdk\Examples\ExamplePopup\*.cpp
File sdk\Examples\ExamplePopup\*.dsp
File sdk\Examples\ExamplePopup\*.png
File sdk\Examples\ExamplePopup\*.rc

SetOutPath $INSTDIR\Examples\ExamplePrefsGroup
File sdk\Examples\ExamplePrefsGroup\*.h
File sdk\Examples\ExamplePrefsGroup\*.cpp
File sdk\Examples\ExamplePrefsGroup\*.dsp

SetOutPath $INSTDIR\Examples\ExampleScriptObject
File sdk\Examples\ExampleScriptObject\*.h
File sdk\Examples\ExampleScriptObject\*.cpp
File sdk\Examples\ExampleScriptObject\*.dsp
File sdk\Examples\ExampleScriptObject\*.rc

SetOutPath $INSTDIR\Examples\ExampleScriptObject\maki
File sdk\Examples\ExampleScriptObject\maki\*.*

SetOutPath $INSTDIR\Examples\ExampleVidSource
File sdk\Examples\ExampleVidSource\*.h
File sdk\Examples\ExampleVidSource\*.cpp
File sdk\Examples\ExampleVidSource\*.dsp

SetOutPath $INSTDIR\Examples\ExampleVisData
File sdk\Examples\ExampleVisData\*.h
File sdk\Examples\ExampleVisData\*.cpp
File sdk\Examples\ExampleVisData\*.dsp
File sdk\Examples\ExampleVisData\*.png
File sdk\Examples\ExampleVisData\*.rc

SetOutPath $INSTDIR\Examples\Standalone
File sdk\Examples\Standalone\*.h
File sdk\Examples\Standalone\*.cpp
File sdk\Examples\Standalone\*.dsp
File sdk\Examples\Standalone\*.dsw

SetOutPath $INSTDIR\Examples\Standalone\Cfg
File sdk\Examples\Standalone\*.*

SetOutPath $INSTDIR\Examples\Standalone\Cfg\Minimum
File sdk\Examples\Standalone\Cfg\Minimum\*.*

SetOutPath $INSTDIR\Examples\Standalone\Cfg\Timers
File sdk\Examples\Standalone\Cfg\Timers\*.*

#SetOutPath $INSTDIR\Examples\ExampleX
#File sdk\Examples\ExampleX\*.cpp
#File sdk\Examples\ExampleX\*.dsp
#File sdk\Examples\ExampleX\*.xml



SetOutPath $INSTDIR\Examples\GenericWACTemplate
File sdk\Examples\GenericWACTemplate\*.h
File sdk\Examples\GenericWACTemplate\*.cpp
File sdk\Examples\GenericWACTemplate\*.dsp
File sdk\Examples\GenericWACTemplate\*.png
File sdk\Examples\GenericWACTemplate\*.rc

SetOutPath $INSTDIR\Examples\xuiobject_template
File sdk\Examples\xuiobject_template\*.exe
File sdk\Examples\xuiobject_template\*.cpp
File sdk\Examples\xuiobject_template\*.dsp
File sdk\Examples\xuiobject_template\*.h
File sdk\Examples\xuiobject_template\*.bat
File sdk\Examples\xuiobject_template\*.rc
File sdk\Examples\xuiobject_template\*.txt
File sdk\Examples\xuiobject_template\*.xml


SetOutPath $INSTDIR\studio\wacs\ExampleMW
File sdk\studio\wacs\exampleMW\*.*

SetOutPath $INSTDIR\studio\wacs\ExampleExternElement
File sdk\studio\wacs\ExampleExternElement\*.*

SetOutPath $INSTDIR\studio\wacs\ExampleHButton
File sdk\studio\wacs\ExampleHButton\*.*

SetOutPath $INSTDIR\studio\wacs\ExampleHTTPXML
File sdk\studio\wacs\exampleHTTPXML\*.*

SetOutPath $INSTDIR\studio\wacs\ExamplePrefsGroup
File sdk\studio\wacs\ExamplePrefsGroup\*.*

SetOutPath $INSTDIR\studio\wacs\FreeFormComponentExample
File sdk\studio\wacs\FreeFormComponentExample\*.*

SectionEnd

SubSection "wasabi.system components"

Section "Addons manager"
SectionIn 1
SetOutPath $INSTDIR\addons
File sdk\addons\*.h
File sdk\addons\*.cpp
File sdk\addons\*.dsp
File sdk\addons\*.rc
SectionEnd

Section "Configurification Registry"
SectionIn 1
SetOutPath $INSTDIR\config
File sdk\config\*.h
File sdk\config\*.cpp
File sdk\config\*.dsp
File sdk\config\*.rc
File sdk\config\*.png
SectionEnd

Section "Guiobjects.wac (GUI object implementations)"
SectionIn 1
SetOutPath $INSTDIR\guiobjects
File sdk\guiobjects\*.h
File sdk\guiobjects\*.cpp
File sdk\guiobjects\*.dsp
SectionEnd

Section "HTTP reader"
SectionIn 1
SetOutPath $INSTDIR\httpread
File sdk\httpread\*.h
File sdk\httpread\*.cpp
File sdk\httpread\*.dsp
File sdk\httpread\*.rc
SectionEnd

Section "Imggen.wac (image generators)"
SectionIn 1
SetOutPath $INSTDIR\imggen
File sdk\imggen\*.h
File sdk\imggen\*.cpp
File sdk\imggen\*.dsp
SectionEnd

Section "JPEG loader"
SectionIn 1
SetOutPath $INSTDIR\jpgload
File sdk\jpgload\*.h
File sdk\jpgload\*.cpp
File sdk\jpgload\*.dsp
#someday once we don't autoprefix files SetOutPath $INSTDIR\jpgload\jpgdlib
#someday once we don't autoprefix files File sdk\jpgload\jpgdlib\*.*
SectionEnd

Section "JPEG saver"
SectionIn 1
SetOutPath $INSTDIR\jpgsave
File sdk\jpgsave\*.h
File sdk\jpgsave\*.cpp
File sdk\jpgsave\*.dsp
SectionEnd

Section "Minibrowser"
SectionIn 1
SetOutPath $INSTDIR\mbsvc
File sdk\mbsvc\*.h
File sdk\mbsvc\*.cpp
File sdk\mbsvc\*.dsp
SectionEnd

Section "PNG loader"
SectionIn 1
SetOutPath $INSTDIR\pngload
File sdk\pngload\*.h
File sdk\pngload\*.cpp
File sdk\pngload\*.dsp
SectionEnd

Section "PPM loader"
SectionIn 1
SetOutPath $INSTDIR\ppmload
File sdk\ppmload\*.h
File sdk\ppmload\*.cpp
File sdk\ppmload\*.dsp
SectionEnd

Section "Preferences window"
SectionIn 1
SetOutPath $INSTDIR\preferences
File sdk\preferences\*.h
File sdk\preferences\*.cpp
File sdk\preferences\*.dsp
File sdk\preferences\*.rc
File sdk\preferences\*.png
SectionEnd

Section "Skinswitcher"
SectionIn 1
SetOutPath $INSTDIR\skinswitch
File sdk\skinswitch\*.h
File sdk\skinswitch\*.cpp
File sdk\skinswitch\*.dsp
File sdk\skinswitch\*.rc
File sdk\skinswitch\*.png
SectionEnd

Section "Standard services provider"
SectionIn 1
SetOutPath $INSTDIR\stdsvc
File sdk\stdsvc\*.h
File sdk\stdsvc\*.cpp
File sdk\stdsvc\*.rc
File sdk\stdsvc\*.dsp
SectionEnd

Section "UTF8 support"
SectionIn 1
SetOutPath $INSTDIR\str_win32_utf8
File sdk\str_win32_utf8\*.h
File sdk\str_win32_utf8\*.cpp
File sdk\str_win32_utf8\*.dsp
SectionEnd

SubSectionEnd # wasabi.system

SubSection "wasabi.player components"

Section "Audio normalizer"
SectionIn 1
SetOutPath $INSTDIR\normal
File sdk\normal\*.h
File sdk\normal\*.cpp
File sdk\normal\*.dsp
SetOutPath $INSTDIR\normal\wavgain
File sdk\normal\wavgain\*.*
SectionEnd

Section "Crossfader"
SectionIn 1
SetOutPath $INSTDIR\cnv_crossfade
File sdk\cnv_crossfade\*.h
File sdk\cnv_crossfade\*.cpp
File sdk\cnv_crossfade\*.dsp
SectionEnd

Section "DirectSound output"
SectionIn 1
SetOutPath $INSTDIR\cnv_pcmdsound
File sdk\cnv_pcmdsound\*.h
File sdk\cnv_pcmdsound\*.cpp
File sdk\cnv_pcmdsound\*.dsp
File sdk\cnv_pcmdsound\*.rc
SectionEnd

Section "DirectShow output"
SectionIn 1
SetOutPath $INSTDIR\cnv_dshow
File sdk\cnv_pcmdsound\*.h
File sdk\cnv_pcmdsound\*.cpp
File sdk\cnv_pcmdsound\*.dsp
SectionEnd

#SetOutPath $INSTDIR\console
#File sdk\console\*.h
#File sdk\console\*.cpp
#File sdk\console\*.dsp

Section "Infoedit.wac (file info editor)"
SectionIn 1
SetOutPath $INSTDIR\infoedit
File sdk\infoedit\*.h
File sdk\infoedit\*.cpp
File sdk\infoedit\*.dsp
File sdk\infoedit\*.png
SectionEnd

Section "Raw PCM reader"
SectionIn 1
SetOutPath $INSTDIR\cnv_rawpcm
File sdk\cnv_rawpcm\*.h
File sdk\cnv_rawpcm\*.cpp
File sdk\cnv_rawpcm\*.dsp
SectionEnd

Section "Core Scripting"
SectionIn 1
SetOutPath $INSTDIR\scriptcore
File sdk\scriptcore\*.h
File sdk\scriptcore\*.cpp
File sdk\scriptcore\*.dsp
File sdk\scriptcore\*.rc
SectionEnd

Section "EQ Manager"
SectionIn 1
SetOutPath $INSTDIR\eqmgr
File sdk\eqmgr\*.h
File sdk\eqmgr\*.cpp
File sdk\eqmgr\*.dsp
File sdk\eqmgr\*.rc
SectionEnd

Section "Media Library"
SectionIn 1
SetOutPath $INSTDIR\library
File sdk\library\*.h
File sdk\library\*.cpp
File sdk\library\*.dsp
SectionEnd

Section "Playlist Editor"
SectionIn 1
SetOutPath $INSTDIR\pledit
File sdk\pledit\*.h
File sdk\pledit\*.cpp
File sdk\pledit\*.dsp
SectionEnd

Section "Shoutcaster"
SectionIn 1
SetOutPath $INSTDIR\shoutcast
File sdk\shoutcast\*.h
File sdk\shoutcast\*.cpp
File sdk\shoutcast\*.dsp
File sdk\shoutcast\*.png
File sdk\shoutcast\*.rc
SetOutPath $INSTDIR\shoutcast\ogg
File sdk\shoutcast\ogg\*.h
SetOutPath $INSTDIR\shoutcast\vorbis
File sdk\shoutcast\vorbis\*.h
SectionEnd

Section "Tone generator"
SectionIn 1
SetOutPath $INSTDIR\cnv_tonepcm
File sdk\cnv_tonepcm\*.h
File sdk\cnv_tonepcm\*.cpp
File sdk\cnv_tonepcm\*.dsp
SectionEnd

Section "Video window"
SectionIn 1
SetOutPath $INSTDIR\video
File sdk\video\*.h
File sdk\video\*.cpp
File sdk\video\*.dsp
File sdk\video\*.rc
File sdk\video\*.png
SectionEnd

Section "Vorbis decoder"
SectionIn 1
SetOutPath $INSTDIR\cnv_floatpcm
File sdk\cnv_floatpcm\*.h
File sdk\cnv_floatpcm\*.cpp
File sdk\cnv_floatpcm\*.dsp
SetOutPath $INSTDIR\cnv_vorbis
File sdk\cnv_vorbis\*.h
File sdk\cnv_vorbis\*.cpp
File sdk\cnv_vorbis\*.dsp
File sdk\cnv_vorbis\*.rc
SectionEnd

#Section "VNC server"
#SectionIn 1
#SetOutPath $INSTDIR\vnc
#File sdk\vnc\*.h
#File sdk\vnc\*.cpp
#File sdk\vnc\*.dsp
#SectionEnd

Section "WAV file reader"
SectionIn 1
SetOutPath $INSTDIR\cnv_wavpcm
File sdk\cnv_wavpcm\*.h
File sdk\cnv_wavpcm\*.cpp
File sdk\cnv_wavpcm\*.dsp
SectionEnd

Section "WAVEOUT output"
SectionIn 1
SetOutPath $INSTDIR\cnv_pcmwaveout
File sdk\cnv_pcmwaveout\*.h
File sdk\cnv_pcmwaveout\*.cpp
File sdk\cnv_pcmwaveout\*.dsp
SectionEnd

Section "WINAMP 2.x skin emulation"
SectionIn 1
SetOutPath $INSTDIR\wa2skin
File sdk\wa2skin\*.h
File sdk\wa2skin\*.cpp
File sdk\wa2skin\*.dsp
SectionEnd

SubSectionEnd # wasabi.player


Section "Create Start Menu items"
SetOutPath "$SMPROGRAMS\Wasabi SDK"
CreateShortCut "$SMPROGRAMS\Wasabi SDK\License.lnk" "$INSTDIR\sdklicense.txt" "" "" 0
#CreateShortCut "$SMPROGRAMS\Wasabi SDK\Manual.lnk" "$INSTDIR\docs\overview.abw" "" "" 0
CreateShortCut "$SMPROGRAMS\Wasabi SDK\Uninstall Wasabi SDK.lnk" "$INSTDIR\uninst-wsdk.EXE" "" "" 0
SectionEnd

Section Uninstall
Delete "$SMPROGRAMS\Wasabi SDK\*.lnk"
RMDir  "$SMPROGRAMS\Wasabi SDK"

Delete "$INSTDIR\bfc\attribs\*.*"
RMDir  "$INSTDIR\bfc\attribs"
Delete "$INSTDIR\bfc\api\*.*"
RMDir  "$INSTDIR\bfc\api"
Delete "$INSTDIR\bfc\parse\*.*"
RMDir  "$INSTDIR\bfc\parse"
Delete "$INSTDIR\bfc\platform\win32\*.*"
RMDir  "$INSTDIR\bfc\platform\win32"
Delete "$INSTDIR\bfc\platform\linux\*.*"
RMDir  "$INSTDIR\bfc\platform\linux"
Delete "$INSTDIR\bfc\platform\freetype\*.*"
RMDir  "$INSTDIR\bfc\platform\freetype"
Delete "$INSTDIR\bfc\platform\*.*"
RMDir  "$INSTDIR\bfc\platform"
Delete "$INSTDIR\bfc\wnds\form\*.*"
RMDir  "$INSTDIR\bfc\wnds\form"
Delete "$INSTDIR\bfc\wnds\*.*"
RMDir  "$INSTDIR\bfc\wnds"
Delete "$INSTDIR\bfc\http\*.*"
RMDir  "$INSTDIR\bfc\http"
Delete "$INSTDIR\bfc\feeds\*.*"
RMDir  "$INSTDIR\bfc\feeds"
Delete "$INSTDIR\bfc\draw\*.*"
RMDir  "$INSTDIR\bfc\draw"
Delete "$INSTDIR\bfc\db\*.*"
RMDir  "$INSTDIR\bfc\db"
Delete "$INSTDIR\bfc\util\*.*"
RMDir  "$INSTDIR\bfc\util"
Delete "$INSTDIR\bfc\xml\*.*"
RMDir  "$INSTDIR\bfc\xml"
Delete "$INSTDIR\bfc\cdda\*.*"
RMDir  "$INSTDIR\bfc\cdda"
Delete "$INSTDIR\bfc\*.*"
RMDir  "$INSTDIR\bfc"
Delete "$INSTDIR\common\script\c_script\*.*"
RMDir  "$INSTDIR\common\script\c_script"
Delete "$INSTDIR\common\script\*.*"
RMDir  "$INSTDIR\common\script"
Delete "$INSTDIR\common\*.*"
RMDir  "$INSTDIR\common"
Delete "$INSTDIR\console\*.*"
RMDir  "$INSTDIR\console"
Delete "$INSTDIR\Examples\Example1\*.*"
RMDir  "$INSTDIR\Examples\Example1"
Delete "$INSTDIR\Examples\ExampleAttrib\*.*"
RMDir  "$INSTDIR\Examples\ExampleAttrib"
Delete "$INSTDIR\Examples\ExampleCanvas\*.*"
RMDir  "$INSTDIR\Examples\ExampleCanvas"
Delete "$INSTDIR\Examples\ExampleCodeGUI\*.*"
RMDir  "$INSTDIR\Examples\ExampleCodeGUI"
Delete "$INSTDIR\Examples\ExampleCoreCallback\*.*"
RMDir  "$INSTDIR\Examples\ExampleCoreCallback"
Delete "$INSTDIR\Examples\ExampleDragDrop\*.*"
RMDir  "$INSTDIR\Examples\ExampleDragDrop"
Delete "$INSTDIR\Examples\ExampleDsp\*.*"
RMDir  "$INSTDIR\Examples\ExampleDsp"
Delete "$INSTDIR\Examples\ExampleExternElement\*.*"
RMDir  "$INSTDIR\Examples\ExampleExternElement"
Delete "$INSTDIR\Examples\ExampleFreeFormWac\*.*"
RMDir  "$INSTDIR\Examples\ExampleFreeFormWac"
Delete "$INSTDIR\Examples\ExampleHButton\*.*"
RMDir  "$INSTDIR\Examples\ExampleHButton"
Delete "$INSTDIR\Examples\ExampleHTTPXML\*.*"
RMDir  "$INSTDIR\Examples\ExampleHTTPXML"
Delete "$INSTDIR\Examples\ExampleItemSequencer\*.*"
RMDir  "$INSTDIR\Examples\ExampleItemSequencer"
Delete "$INSTDIR\Examples\ExampleMetaData\*.*"
RMDir  "$INSTDIR\Examples\ExampleMetaData"
Delete "$INSTDIR\Examples\ExampleMW\*.*"
RMDir  "$INSTDIR\Examples\ExampleMW"
Delete "$INSTDIR\Examples\ExampleOSWnd\*.*"
RMDir  "$INSTDIR\Examples\ExampleOSWnd"
Delete "$INSTDIR\Examples\ExamplePaintCB\*.*"
RMDir  "$INSTDIR\Examples\ExamplePaintCB"
Delete "$INSTDIR\Examples\ExamplePl\*.*"
RMDir  "$INSTDIR\Examples\ExamplePl"
Delete "$INSTDIR\Examples\ExamplePopup\*.*"
RMDir  "$INSTDIR\Examples\ExamplePopup"
Delete "$INSTDIR\Examples\ExamplePrefsGroup\*.*"
RMDir  "$INSTDIR\Examples\ExamplePrefsGroup"
Delete "$INSTDIR\Examples\ExampleScriptObject\maki\*.*"
RMDir  "$INSTDIR\Examples\ExampleScriptObject\maki"
Delete "$INSTDIR\Examples\ExampleScriptObject\*.*"
RMDir  "$INSTDIR\Examples\ExampleScriptObject"
Delete "$INSTDIR\Examples\ExampleVidSource\*.*"
RMDir  "$INSTDIR\Examples\ExampleVidSource"
Delete "$INSTDIR\Examples\ExampleVisData\*.*"
RMDir  "$INSTDIR\Examples\ExampleVisData"
Delete "$INSTDIR\Examples\Standalone\Cfg\Minimum\*.*"
RMDir  "$INSTDIR\Examples\Standalone\Cfg\Minimum"
Delete "$INSTDIR\Examples\Standalone\Cfg\*.*"
RMDir  "$INSTDIR\Examples\Standalone\Cfg"
Delete "$INSTDIR\Examples\Standalone*.*"
RMDir  "$INSTDIR\Examples\Standalone"
Delete "$INSTDIR\Examples\GenericWACTemplate\*.*"
RMDir  "$INSTDIR\Examples\GenericWACTemplate"
Delete "$INSTDIR\Examples\xuiobject_template\*.*"
RMDir  "$INSTDIR\Examples\xuiobject_template"
Delete "$INSTDIR\Examples\*.*"
RMDir  "$INSTDIR\Examples"
Delete "$INSTDIR\eqmgr\*.*"
RMDir  "$INSTDIR\eqmgr"
Delete "$INSTDIR\mbsvc\*.*"
RMDir  "$INSTDIR\mbsvc"
Delete "$INSTDIR\jpgload\*.*"
RMDir  "$INSTDIR\jpgload"
Delete "$INSTDIR\jpgsave\*.*"
RMDir  "$INSTDIR\jpgsave"
Delete "$INSTDIR\preferences\*.*"
RMDir  "$INSTDIR\preferences"
Delete "$INSTDIR\str_win32_utf8\*.*"
RMDir  "$INSTDIR\str_win32_utf8"
Delete "$INSTDIR\ppmload\*.*"
RMDir  "$INSTDIR\ppmload"
Delete "$INSTDIR\pngload\*.*"
RMDir  "$INSTDIR\pngload"
Delete "$INSTDIR\pledit\*.*"
RMDir  "$INSTDIR\pledit"
Delete "$INSTDIR\library\*.*"
RMDir  "$INSTDIR\library"
Delete "$INSTDIR\taskband\*.*"
RMDir  "$INSTDIR\taskband"

Delete "$INSTDIR\addons\*.*"
RMDir  "$INSTDIR\addons"
Delete "$INSTDIR\cnv_crossfade\*.*"
RMDir  "$INSTDIR\cnv_crossfade"
Delete "$INSTDIR\cnv_pcmdsound\*.*"
RMDir  "$INSTDIR\cnv_pcmdsound"
Delete "$INSTDIR\cnv_pcmwaveout\*.*"
RMDir  "$INSTDIR\cnv_pcmwaveout"
Delete "$INSTDIR\cnv_tonepcm\*.*"
RMDir  "$INSTDIR\cnv_tonepcm"
Delete "$INSTDIR\cnv_floatpcm\*.*"
RMDir  "$INSTDIR\cnv_floatpcm"
Delete "$INSTDIR\cnv_vorbis\*.*"
RMDir  "$INSTDIR\cnv_vorbis"
Delete "$INSTDIR\cnv_wavpcm\*.*"
RMDir  "$INSTDIR\cnv_wavpcm"
Delete "$INSTDIR\cnv_rawpcm\*.*"
RMDir  "$INSTDIR\cnv_rawpcm"
Delete "$INSTDIR\cnv_dshow\*.*"
RMDir  "$INSTDIR\cnv_dshow"
Delete "$INSTDIR\config\*.*"
RMDir  "$INSTDIR\config"
Delete "$INSTDIR\guiobjects\*.*"
RMDir  "$INSTDIR\guiobjects"
Delete "$INSTDIR\httpread\*.*"
RMDir  "$INSTDIR\httpread"
Delete "$INSTDIR\imggen\*.*"
RMDir  "$INSTDIR\imggen"
Delete "$INSTDIR\core\*.*"
RMDir  "$INSTDIR\core"
Delete "$INSTDIR\jnetlib\*.*"
RMDir  "$INSTDIR\jnetlib"
Delete "$INSTDIR\infoedit\*.*"
RMDir  "$INSTDIR\infoedit"
Delete "$INSTDIR\mbsvc\*.*"
RMDir  "$INSTDIR\mbsvc"
Delete "$INSTDIR\normal\wavgain\*.*"
RMDir  "$INSTDIR\normal\wavgain"
Delete "$INSTDIR\normal\*.*"
RMDir  "$INSTDIR\normal"
Delete "$INSTDIR\scriptcore\*.*"
RMDir  "$INSTDIR\scriptcore"
Delete "$INSTDIR\shoutcast\ogg\*.*"
RMDir  "$INSTDIR\shoutcast\ogg"
Delete "$INSTDIR\shoutcast\vorbis\*.*"
RMDir  "$INSTDIR\shoutcast\vorbis"
Delete "$INSTDIR\shoutcast\*.*"
RMDir  "$INSTDIR\shoutcast"
Delete "$INSTDIR\skinswitch\*.*"
RMDir  "$INSTDIR\skinswitch"
Delete "$INSTDIR\stdsvc\*.*"
RMDir  "$INSTDIR\stdsvc"
Delete "$INSTDIR\video\*.*"
RMDir  "$INSTDIR\video"
Delete "$INSTDIR\vnc\*.*"
RMDir  "$INSTDIR\vnc"
Delete "$INSTDIR\wa2skin\*.*"
RMDir  "$INSTDIR\wa2skin"

Delete "$INSTDIR\bin\templates\data\*.*"
RMDir  "$INSTDIR\bintemplates\data"

Delete "$INSTDIR\bin\templates\*.*"
RMDir  "$INSTDIR\bin\templates"

Delete "$INSTDIR\bin\*.*"
RMDir  "$INSTDIR\bin"

Delete "$INSTDIR\studio\lib\*.*"
RMDir  "$INSTDIR\studio\lib"
Delete "$INSTDIR\studio\script\*.*"
RMDir  "$INSTDIR\studio\script"
Delete "$INSTDIR\studio\Locales\*.*"
RMDir  "$INSTDIR\studio\Locales"
Delete "$INSTDIR\studio\services\*.*"
RMDir  "$INSTDIR\studio\services"
Delete "$INSTDIR\studio\wacs\ExampleExternElement\*.*"
RMDir  "$INSTDIR\studio\wacs\ExampleExternElement"
Delete "$INSTDIR\studio\wacs\ExampleHButton\*.*"
RMDir  "$INSTDIR\studio\wacs\ExampleHButton"
Delete "$INSTDIR\studio\wacs\ExampleHTTPXML\*.*"
RMDir  "$INSTDIR\studio\wacs\ExampleHTTPXML"
Delete "$INSTDIR\studio\wacs\ExampleMW\*.*"
RMDir  "$INSTDIR\studio\wacs\ExampleMW"
Delete "$INSTDIR\studio\wacs\ExamplePrefsGroup\*.*"
RMDir  "$INSTDIR\studio\wacs\ExamplePrefsGroup"
Delete "$INSTDIR\studio\wacs\FreeFormComponentExample\*.*"
RMDir  "$INSTDIR\studio\wacs\FreeFormComponentExample"
Delete "$INSTDIR\studio\wacs\*.*"
RMDir  "$INSTDIR\studio\wacs"
Delete "$INSTDIR\studio\wa3cfg\*.*"
RMDir  "$INSTDIR\studio\wa3cfg"
Delete "$INSTDIR\studio\wa5cfg\*.*"
RMDir  "$INSTDIR\studio\wa5cfg"
Delete "$INSTDIR\studio\xml\*.*"
RMDir  "$INSTDIR\studio\xml"
Delete "$INSTDIR\studio\*.*"
RMDir  "$INSTDIR\studio"
Delete "$INSTDIR\waclient\*.*"
RMDir  "$INSTDIR\waclient"
Delete "$INSTDIR\wasabi\*.*"
Delete "$INSTDIR\wasabi\api\*.*"
RMDir  "$INSTDIR\wasabi\api"
RMDir  "$INSTDIR\wasabi"
Delete "$INSTDIR\video\*.*"
RMDir  "$INSTDIR\video"
Delete "$INSTDIR\Debug\*.*"
RMDir  "$INSTDIR\Debug"
Delete "$INSTDIR\Release\*.*"
RMDir  "$INSTDIR\Release"
Delete "$INSTDIR\docs\*.*"
RMDir  "$INSTDIR\docs"
Delete "$INSTDIR\nsi\*.*"
RMDir  "$INSTDIR\nsi"
Delete "$INSTDIR\*.*"
RMDir  "$INSTDIR"

Delete "$INSTDIR\*.*"
RMDir  "$INSTDIR"
SectionEnd
