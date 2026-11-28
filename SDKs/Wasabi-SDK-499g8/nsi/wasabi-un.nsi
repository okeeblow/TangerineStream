# include this into your uninstall section

!ifndef WASABI_APPNAME
!error "This script needs WASABI_APPNAME to be defined."
!endif

#Push $R1
#Push $R0
#
#ReadRegDWORD $R0 HKLM "SOFTWARE\Nullsoft\Wasabi\${WASABIVER}" "REF_${WASABI_APPNAME}"
#IntOp $R0 $R0 - 1
#WriteRegDWORD HKLM "SOFTWARE\Nullsoft\Wasabi\${WASABIVER}" "REF_${WASABI_APPNAME}" $R0
#
#; if $R0 is not 0 after this is installed, don't do anything.
#IntCmp $R0 0 "" Wasabi_NoUninstall Wasabi_NoUninstall
#
#; our install count is 0, remove ourselves
#DeleteRegValue HKLM "SOFTWARE\Nullsoft\Wasabi\${WASABIVER}" "REF_${WASABI_APPNAME}"
#
#; check to see if anyone else is still around with a REF_*
#
#  StrCpy $R0 "0"
#Wasabi_Enum1:
#  EnumRegValue $R1 HKLM "SOFTWARE\Nullsoft\Wasabi\${WASABIVER}" $R0
#  StrCpy $R1 $R1 4
#  StrCmp $R1 "REF_" Wasabi_NoUninstall
#
#  ; not REF_ and not empty, so keep enumerating
#  IntOp $R0 $R0 + 1
#  StrCmp $R1 "" "" Wasabi_Enum1
#; at this point all regvalues have been enumerated, and none of them 
#; started with REF_, so we should be safe to remove our wasabi version.




#RMDir /r "$PROGRAMFILES\Wasabi\${WASABIVER}"
RMDir /r "$INSTDIR\Wacs\wasabi.system"
RMDir /r "$INSTDIR\Wacs\wasabi.player"

RMDir /r "$INSTDIR\themes\default"
RMDir "$INSTDIR\themes"

# if it's not empty, this will fail harmlessly
#RMDir "$PROGRAMFILES\Wasabi"




#Wasabi_NoUninstall:
#
#Pop $R0
#Pop $R1
