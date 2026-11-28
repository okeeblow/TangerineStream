
# include this as a section
!ifndef WASABI_APPNAME
!error "This script needs WASABI_APPNAME to be defined."
!endif

!define WASABIVER "1.1"

Section "wasabi.system (required)"
SectionIn RO

#SetOutPath "$PROGRAMFILES\Wasabi\${WASABIVER}"
SetOutPath "$INSTDIR"

SetOverwrite on

File studio\wasabi.dll

# theme
SetOutPath "$INSTDIR"
File /r studio\themes

#SetOutPath "$PROGRAMFILES\Wasabi\${WASABIVER}\Wacs\wasabi.system"
SetOutPath "$INSTDIR\Wacs\wasabi.system"
# all wasabi installs have these
File /r studio\wacs\wasabi.system\db
File /r studio\wacs\wasabi.system\etc
File /r studio\wacs\wasabi.system\eval
File /r studio\wacs\wasabi.system\i18l
File /r studio\wacs\wasabi.system\img
File /r studio\wacs\wasabi.system\net\httpread
File /r studio\wacs\wasabi.system\ui
File /r studio\wacs\wasabi.system\platform

!ifdef wasabi_player
#SetOutPath "$PROGRAMFILES\Wasabi\${WASABIVER}\Wacs\wasabi.player"
SetOutPath "$INSTDIR\Wacs\wasabi.player"
File /r studio\wacs\wasabi.player\library
File /r studio\wacs\wasabi.player\pledit
File /r studio\wacs\wasabi.player\scriptcore
File /r studio\wacs\wasabi.player\mediacore\core
File /r studio\wacs\wasabi.player\mediacore\cnv\crossfade
File /r studio\wacs\wasabi.player\mediacore\cnv\pcmwaveout
File /r studio\wacs\wasabi.player\mediacore\cnv\pcmdsound
!endif

#WriteRegStr HKLM "SOFTWARE\Nullsoft\Wasabi\${WASABIVER}" "dir" "$PROGRAMFILES\Wasabi\${WASABIVER}"
#Push $R0
#ReadRegDWORD $R0 HKLM "SOFTWARE\Nullsoft\Wasabi\${WASABIVER}" "REF_${WASABI_APPNAME}"
#; if this gives an empty string, adding 1 to it will result in 1 =)
#IntOp $R0 $R0 + 1
#WriteRegDWORD HKLM "SOFTWARE\Nullsoft\Wasabi\${WASABIVER}" "REF_${WASABI_APPNAME}" $R0
#Pop $R0
#
SectionEnd

!ifdef wasabi_player
SubSection "wasabi.player"
  SubSection "Audio Filetypes"
    Section "MP3, MP2"
    SectionIn 1
      File /r studio\wacs\wasabi.player\mediacore\cnv\mp3pcm
    SectionEnd

    Section "CD Audio"
    SectionIn 1
      File /r studio\wacs\wasabi.player\mediacore\cnv\cddapcm
    SectionEnd

    Section "WAV"
    SectionIn 1
      File /r studio\wacs\wasabi.player\mediacore\cnv\wavpcm
    SectionEnd

    Section "OGG Vorbis"
    SectionIn 1
      File /r studio\wacs\wasabi.player\mediacore\cnv\floatpcm
      File /r studio\wacs\wasabi.player\mediacore\cnv\vorbis
    SectionEnd

    Section "MIDI"
    SectionIn 1
      File /r studio\wacs\wasabi.player\mediacore\cnv\midi
    SectionEnd

    Section "MOD/IT/XM etc"
    SectionIn 1
      File /r studio\wacs\wasabi.player\mediacore\cnv\mod
    SectionEnd
  SubSectionEnd #audio filetypes

  Section "Video playback"
  SectionIn 1
    File /r studio\wacs\wasabi.player\video
    File /r studio\wacs\wasabi.player\mediacore\cnv\dshow
    File /r studio\wacs\wasabi.player\mediacore\cnv\nsv
    File /r studio\wacs\wasabi.player\mediacore\cnv\vp3
  SectionEnd

  Section "Minibrowser"
  SectionIn 1
    File /r studio\wacs\wasabi.system\mb
  SectionEnd

;  Section "CD and DVD-R burning"
;  SectionIn 1
;    File /r studio\wacs\wasabi.system\mediarecord
;  SectionEnd

SubSectionEnd
!endif


