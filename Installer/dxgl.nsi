; DXGL
; Copyright (C) 2011-2025 William Feely

; This library is free software; you can redistribute it and/or
; modify it under the terms of the GNU Lesser General Public
; License as published by the Free Software Foundation; either
; version 2.1 of the License, or (at your option) any later version.

; This library is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
; Lesser General Public License for more details.

; You should have received a copy of the GNU Lesser General Public
; License along with this library; if not, write to the Free Software
; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

; Temporary fix for NSIS 3.07+
Unicode False

SetCompressor /SOLID lzma
!ifdef NSIS_PACKEDVERSION
  ManifestSupportedOS all
  ManifestDPIAware true
!endif

!include 'WinVer.nsh'
!include 'LogicLib.nsh'
!include "WordFunc.nsh"
!include 'x64.nsh'
!include "..\common\version.nsh"

; Product name is different for x64
!ifdef _CPU_X64
!define PRODUCT_NAME "DXGL x64"
!define SMDIR "DXGL x64"
!else
!define PRODUCT_NAME "DXGL"
!define SMDIR "DXGL"
!endif
!define PRODUCT_PUBLISHER "William Feely"
!define PRODUCT_WEB_SITE "https://dxgl.org/"
!ifdef _CPU_X64
!define PRODUCT_DIR_REGKEY "Software\DXGL"
!else
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\dxglcfg.exe"
!endif
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

!if ${COMPILER} == "VC2022_14"
!ifdef _DEBUG
!define PLUGINDIR "Debug VS2022"
!ifdef _CPU_X64
!define SRCDIR "x64\Debug VS2022"
!else
!define SRCDIR "Debug VS2022"
!endif
!else
!define PLUGINDIR "Release VS2022"
!ifdef _CPU_X64
!define SRCDIR "x64\Release VS2022"
!else
!define SRCDIR "Release VS2022"
!endif
!endif
!else if ${COMPILER} == "VC2005"
!ifdef _DEBUG
!define PLUGINDIR "VS8\Debug"
!define SRCDIR "VS8\Debug"
!else
!define PLUGINDIR "VS8\Release"
!define SRCDIR "VS8\Release"
!endif
!else if ${COMPILER} == "VC2008"
!ifdef _DEBUG
!define PLUGINDIR "VS9\Debug"
!define SRCDIR "VS9\Debug"
!else
!define PLUGINDIR "VS9\Release"
!define SRCDIR "VS9\Release"
!endif
!else
!ifdef _DEBUG
!define PLUGINDIR "Debug"
!ifdef _CPU_X64
!define SRCDIR "x64\Debug"
!else
!define SRCDIR "Debug"
!endif
!else
!define PLUGINDIR "Release"
!ifdef _CPU_X64
!define SRCDIR "x64\Release"
!else
!define SRCDIR "Release"
!endif
!endif
!endif

; MUI2
!define MUI_BGCOLOR "SYSCLR:Window"
!include "MUI2.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "..\common\dxgl48.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

var RadioButtonPortable
var RadioButtonNormal
var PortableMode

Function PortableModeEnter
	nsDialogs::Create 1018
	!insertmacro MUI_HEADER_TEXT "Installation mode" "Do you want to install DXGL in portable or normal mode?"
	; === RadioButtonPortable (type: RadioButton) ===
	${NSD_CreateRadioButton} 16u 39u 221u 15u "Portable mode:  Extracts DXGL without updating the registry"
	Pop $RadioButtonPortable
	${NSD_AddStyle} $RadioButtonPortable ${WS_GROUP}
	; === RadioButtonNormal (type: RadioButton) ===
	${NSD_CreateRadioButton} 16u 20u 175u 15u "Normal mode:  Install DXGL and update profiles"
	Pop $RadioButtonNormal
	${If} $PortableMode == "0"
		${NSD_Check} $RadioButtonNormal
	${Else}
		${NSD_Check} $RadioButtonPortable
	${EndIF}
	nsDialogs::Show
FunctionEnd

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!define MUI_LICENSEPAGE_TEXT_BOTTOM "DXGL is licensed under the LGPLv2+.  Click next to continue."
!define MUI_LICENSEPAGE_BUTTON "Next >"
!insertmacro MUI_PAGE_LICENSE "..\COPYING.txt"
; Portable mode page
Page Custom PortableModeEnter PortableModeLeave
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Components page
!define MUI_COMPONENTSPAGE_SMALLDESC
!insertmacro MUI_PAGE_COMPONENTS
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\dxglcfg.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Configure DXGL"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\ReadMe.txt"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"


; MUI end ------

!define HKEY_CURRENT_USER 0x80000001
!define RegOpenKeyEx     "Advapi32::RegOpenKeyEx(i, t, i, i, *i) i"
!define RegQueryValueEx  "Advapi32::RegQueryValueEx(i, t, i, *i, i, *i) i"
!define RegCloseKey      "Advapi32::RegCloseKey(i) i"
!define REG_MULTI_SZ     7
!define INSTPATH         "InstallPaths"
!define KEY_QUERY_VALUE          0x0001
!define KEY_ENUMERATE_SUB_KEYS   0x0008
!define ROOT_KEY         ${HKEY_CURRENT_USER}
!define GetVersion       "Kernel32::GetVersion() i"

!ifdef _CPU_X64
!define PRODUCT_PLATFORM "x64"
!if ${COMPILER} == "VC2005"
!define download_runtime 1
!define runtime_url "http://dxgl.org/download/runtimes/vc8-6195/vcredist_x64.EXE"
!define runtime_name "Visual C++ 2005 x64"
!define runtime_filename "vcredist_x64.EXE"
!define runtime_sha512 "F8E15363E34DB5B5445C41EEA4DD80B2F682642CB8F1046F30EA4FB5F4F51B0B604F7BCB3000A35A7D3BA1D1BCC07DF9B25E4533170C65640B2D137C19916736"
!define runtime_regkey SOFTWARE\Microsoft\DevDiv\VC\Servicing\8.0\RED\1033
!define runtime_regvalue Install
!define PRODUCT_SUFFIX "-msvc8"
!else if ${COMPILER} == "VC2008"
!define download_runtime 1
!define runtime_url "http://dxgl.org/download/runtimes/vc9-6161/vcredist_x64.exe"
!define runtime_name "Visual C++ 2008 x64"
!define runtime_filename "vcredist_x64.exe"
!define runtime_sha512 "B890D83D36F3681A690828D8926139B4F13F8D2FCD258581542CF2FB7DCE5D7E7E477731C9545A54A476ED5C2AAAC44CE12D2C3D9B99C2C1C04A5AB4EE20C4B8"
!define runtime_regkey SOFTWARE\Microsoft\DevDiv\VC\Servicing\9.0\RED\1033
!define runtime_regvalue Install
!define PRODUCT_SUFFIX "-msvc9"
!else if ${COMPILER} == "VC2010"
!define download_runtime 1
!define runtime_url "http://dxgl.org/download/runtimes/vc10/vcredist_x64.exe"
!define runtime_name "Visual C++ 2010 x64"
!define runtime_filename "vcredist_x64.exe"
!define runtime_sha512 "24B56B5D9B48D75BAF53A98E007ACE3E7D68FBD5FA55B75AE1A2C08DD466D20B13041F80E84FDB64B825F070843F9247DABA681EFF16BAF99A4B14EA99F5CFD6"
!define runtime_regkey SOFTWARE\Microsoft\VisualStudio\10.0\VC\VCRedist\x64
!define runtime_regvalue Installed
!define PRODUCT_SUFFIX "-msvc10"
!else if ${COMPILER} == "VC2013"
!define download_runtime 1
!define runtime_url "http://dxgl.org/download/runtimes/vc12/vcredist_x64.exe"
!define runtime_name "Visual C++ 2013 x64"
!define runtime_filename "vcredist_x64.exe"
!define runtime_sha512 "3A55DCE14BBD455808BD939A5008B67C9C7111CAB61B1339528308022E587726954F8C55A597C6974DC543964BDB6532FE433556FBEEAF9F8CB4D95F2BBFFC12"
!define runtime_regkey SOFTWARE\Microsoft\DevDiv\vc\Servicing\12.0\RuntimeMinimum
!define runtime_regvalue Install
!define PRODUCT_SUFFIX "-msvc12"
!else if ${COMPILER} == "VC2022_14"
!define download_runtime 1
!define runtime_url http://dxgl.org/download/runtimes/vc14.44.35112/vc_redist.x64.exe
!define runtime_name "Visual C++ 2022.14 x64"
!define runtime_filename "vc_redist.x64.exe"
!define runtime_sha512 "610631F92193015D32D572A8BCD18792EC7FD898F28BD4F69E7708D0989AB51FBAA0EB306D40EF5CBEAA2242BFBA4639C8881AD09697FC5E8117F5B856602BB9"
!define runtime_regkey SOFTWARE\Microsoft\DevDiv\vc\Servicing\14.0\RuntimeMinimum
!define runtime_regvalue Install
!define runtime_regvalue2 Version
!define PRODUCT_SUFFIX ""
!else
!define download_runtime 0
!endif
!else
!define PRODUCT_PLATFORM "win32"
!if ${COMPILER} == "VC2005"
!define download_runtime 1
!define runtime_url "http://dxgl.org/download/runtimes/vc8-6195/vcredist_x86.EXE"
!define runtime_name "Visual C++ 2005 x86"
!define runtime_filename "vcredist_x86.EXE"
!define runtime_sha512 "E94B077E054BD8992374D359F3ADC4D1D78D42118D878556715D77182F7D03635850B2B2F06C012CCB7C410E2B3C124CF6508473EFE150D3C51A51857CE1C6B0"
!define runtime_regkey SOFTWARE\Microsoft\DevDiv\VC\Servicing\8.0\RED\1033
!define runtime_regvalue Install
!define PRODUCT_SUFFIX "-msvc8"
!else if ${COMPILER} == "VC2008"
!define download_runtime 1
!define runtime_url "http://dxgl.org/download/runtimes/vc9-6161/vcredist_x86.exe"
!define runtime_name "Visual C++ 2008 x86"
!define runtime_filename "vcredist_x86.exe"
!define runtime_sha512 "BF630667C87B8F10EF85B61F2F379D7CE24124618B999BABFEC8E2DF424EB494B8F1BF0977580810DFF5124D4DBDEC9539FF53E0DC14625C076FA34DFE44E3F2"
!define runtime_regkey SOFTWARE\Microsoft\DevDiv\VC\Servicing\9.0\RED\1033
!define runtime_regvalue Install
!define PRODUCT_SUFFIX "-msvc9"
!else if ${COMPILER} == "VC2010"
!define download_runtime 1
!define runtime_url "http://dxgl.org/download/runtimes/vc10/vcredist_x86.exe"
!define runtime_name "Visual C++ 2010 x86"
!define runtime_filename "vcredist_x86.exe"
!define runtime_sha512 "D2D99E06D49A5990B449CF31D82A33104A6B45164E76FBEB34C43D10BCD25C3622AF52E59A2D4B7F5F45F83C3BA4D23CF1A5FC0C03B3606F42426988E63A9770"
!define runtime_regkey SOFTWARE\Microsoft\VisualStudio\10.0\VC\VCRedist\x86
!define runtime_regvalue Installed
!define PRODUCT_SUFFIX "-msvc10"
!else if ${COMPILER} == "VC2013"
!define download_runtime 1
!define runtime_url "http://dxgl.org/download/runtimes/vc12/vcredist_x86.exe"
!define runtime_name "Visual C++ 2013 x86"
!define runtime_filename "vcredist_x86.exe"
!define runtime_sha512 "729251371ED208898430040FE48CABD286A5671BD7F472A30E9021B68F73B2D49D85A0879920232426B139520F7E21321BA92646985216BF2F733C64E014A71D"
!define runtime_regkey SOFTWARE\Microsoft\DevDiv\vc\Servicing\12.0\RuntimeMinimum
!define runtime_regvalue Install
!define PRODUCT_SUFFIX "-msvc12"
!else if ${COMPILER} == "VC2022_14"
!define download_runtime 1
!define runtime_url http://dxgl.org/download/runtimes/vc14.44.35112/vc_redist.x86.exe
!define runtime_name "Visual C++ 2022.14 x86"
!define runtime_filename "VC_redist.x86.exe"
!define runtime_sha512 "369D9999A5A1237FD9DDA89335CCD7837938E2E76D7A6B9BF0A91DD4002A82EAEE952812840F86C5F00CE5BDB481CCE1AAE1CB5B8AE14A30C0E4EB65C020A38F"
!define runtime_regkey SOFTWARE\Microsoft\DevDiv\vc\Servicing\14.0\RuntimeMinimum
!define runtime_regvalue Install
!define runtime_regvalue2 Version
!define PRODUCT_SUFFIX ""
!else
!define download_runtime 0
!endif
!endif

!addplugindir "..\${PLUGINDIR}"

!ifdef _DEBUG
Name "${PRODUCT_NAME} ${PRODUCT_VERSION} DEBUG BUILD"
!else
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
!endif
!ifdef _DEBUG
OutFile "DXGL-${PRODUCT_VERSION}-${PRODUCT_PLATFORM}-Debug${PRODUCT_SUFFIX}.exe"
!else
OutFile "DXGL-${PRODUCT_VERSION}-${PRODUCT_PLATFORM}${PRODUCT_SUFFIX}.exe"
!endif
!ifdef _CPU_X64
InstallDir "$PROGRAMFILES64\DXGL"
!else
InstallDir "$PROGRAMFILES\DXGL"
!endif
!ifdef _CPU_X64
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" "InstallDir_x64"
!else
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
!endif
ShowInstDetails show
ShowUnInstDetails show

VIProductVersion "${PRODUCT_VERSION_NUMBER}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "DXGL ${PRODUCT_VERSION} Installer"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${PRODUCT_VERSION}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "InternalName" "DXGL"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "Copyright © 2011-2024 William Feely"
VIAddVersionKey /LANG=${LANG_ENGLISH} "OriginalFilename" "DXGL-${PRODUCT_VERSION}-win32.exe"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "DXGL"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" "${PRODUCT_VERSION}"

Section "DXGL Components (required)" SEC01
  SectionIn RO
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer
  Delete "$INSTDIR\dxgltest.exe"
  File "..\${SRCDIR}\dxglcfg.exe"
  File "..\${SRCDIR}\ddraw.dll"
  File /oname=ReadMe.txt "..\ReadMe.md"
  File "..\ThirdParty.txt"
  File "..\COPYING.txt"
  File "..\Help\dxgl.chm"
  File "..\dxgl-example.ini"
  ${If} $PortableMode == "0"
    CreateDirectory "$SMPROGRAMS\${SMDIR}"
    Delete "$SMPROGRAMS\${SMDIR}\DXGL Test.lnk"
    !ifdef _CPU_X64
    CreateShortCut "$SMPROGRAMS\${SMDIR}\Configure DXGL (x64).lnk" "$INSTDIR\dxglcfg.exe"
    !else
    CreateShortCut "$SMPROGRAMS\${SMDIR}\Configure DXGL.lnk" "$INSTDIR\dxglcfg.exe"
    !endif
    CreateShortCut "$SMPROGRAMS\${SMDIR}\Third-party Credits.lnk" "$INSTDIR\ThirdParty.txt"
    CreateShortCut "$SMPROGRAMS\${SMDIR}\DXGL Help.lnk" "$INSTDIR\dxgl.chm"
    CreateShortCut "$SMPROGRAMS\${SMDIR}\Example configuration file.lnk" "$INSTDIR\dxgl-example.ini"
    !ifdef _CPU_X64
    WriteRegStr HKLM "Software\DXGL" "InstallDir_x64" "$INSTDIR"
    !else
    WriteRegStr HKLM "Software\DXGL" "InstallDir" "$INSTDIR"
    !endif
  ${EndIf}
SectionEnd

!ifndef _DEBUG
!if ${download_runtime} >= 1
Section "Download ${runtime_name} Redistributable" SEC_VCREDIST
  ${If} $PortableMode == "0"
    vcdownloadretry:
    DetailPrint "Downloading ${runtime_name} Runtime"
    NSISdl::download ${runtime_url} $TEMP\${runtime_filename}
    DetailPrint "Checking ${runtime_name} Runtime"
    dxgl-nsis::CalculateSha512Sum $TEMP\${runtime_filename} ${runtime_sha512}
    Pop $0
    ${If} $0 == "0"
      MessageBox MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2 "Failed to download ${runtime_name} Redistributable.  Would you like to retry?" IDYES vcdownloadretry
      Delete $TEMP\vcredist_x86.exe
    ${Else}
      DetailPrint "Installing ${runtime_name} Runtime"
      !if ${COMPILER} == "VC2005"
      ExecWait '"$TEMP\${runtime_filename}" /q' $0
      !else if ${COMPILER} == "VC2008"
      ExecWait '"$TEMP\${runtime_filename}" /q' $0
      !else
      ExecWait '"$TEMP\${runtime_filename}" /q /norestart' $0
      !endif
	  ${If} $0 != "0"
	    ${If} $0 == "3010"
	    SetRebootFlag true
	    goto vcinstallcomplete
	    ${Else}
	    MessageBox MB_OK|MB_ICONSTOP "Failed to install ${runtime_name} Redistributable.  Click OK to attempt manual installation."
	    ${EndIf}
      ${Else}
	  goto vcinstallcomplete
	  ${EndIf}
	  ExecWait '"$TEMP\${runtime_filename}"'
	  vcinstallcomplete:
      Delete $TEMP\${runtime_filename}
    ${EndIf}
  ${EndIf}
SectionEnd
!endif
!endif

Section -PostInstall
  ${If} $PortableMode == "0"
    CreateDirectory $INSTDIR\Temp
    CopyFiles $INSTDIR\dxglcfg.exe $INSTDIR\Temp
    ExecWait '"$INSTDIR\Temp\dxglcfg.exe" upgrade'
    ExecWait '"$INSTDIR\dxglcfg.exe" profile_install'
    RMDir /r /REBOOTOK $INSTDIR\Temp
  ${EndIf}
SectionEnd

Section "Set Wine DLL Overrides" SEC_WINEDLLOVERRIDE
  ${If} $PortableMode == "0"
    DetailPrint "Setting Wine DLL Overrides"
    WriteRegDWORD HKLM "Software\DXGL" "WineDLLOverride" 1
    WriteRegStr HKCU "Software\Wine\DllOverrides" "ddraw" "native,builtin"
  ${EndIf}
SectionEnd

Section "Fix DDraw COM registration" SEC_COMFIX
  ${If} $PortableMode == "0"
    DetailPrint "Setting DDraw Runtime path in registry"
    WriteRegDWORD HKLM "Software\DXGL" "COMFix" 1
    !ifndef _CPU_X64
    ${If} ${RunningX64}
    SetRegView 32
    ${EndIf}
    !endif
    WriteRegStr HKCU "Software\Classes\CLSID\{D7B70EE0-4340-11CF-B063-0020AFC2CD35}\InprocServer32" "" "ddraw.dll"
    WriteRegStr HKCU "Software\Classes\CLSID\{D7B70EE0-4340-11CF-B063-0020AFC2CD35}\InprocServer32" "ThreadingModel" "Both"
    WriteRegStr HKCU "Software\Classes\CLSID\{3C305196-50DB-11D3-9CFE-00C04FD930C5}\InprocServer32" "" "ddraw.dll"
    WriteRegStr HKCU "Software\Classes\CLSID\{3C305196-50DB-11D3-9CFE-00C04FD930C5}\InprocServer32" "ThreadingModel" "Both"
    WriteRegStr HKCU "Software\Classes\CLSID\{593817A0-7DB3-11CF-A2DE-00AA00B93356}\InprocServer32" "" "ddraw.dll"
    WriteRegStr HKCU "Software\Classes\CLSID\{593817A0-7DB3-11CF-A2DE-00AA00B93356}\InprocServer32" "ThreadingModel" "Both"
  ${EndIf}
SectionEnd

Section /o "Debug symbols" SEC_DEBUGSYMBOLS
  !if ${COMPILER} == "VC2022_14"
  File "..\${SRCDIR}\cfgmgr.pdb"
  !endif
  File "..\${SRCDIR}\ddraw.pdb"
  File "..\${SRCDIR}\dxglcfg.pdb"
SectionEnd

Section -AdditionalIcons
  ${If} $PortableMode == "0"
    WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
    CreateShortCut "$SMPROGRAMS\${SMDIR}\Website.lnk" "$INSTDIR\${PRODUCT_NAME}.url"
    CreateShortCut "$SMPROGRAMS\${SMDIR}\Uninstall.lnk" "$INSTDIR\uninst.exe"
  ${EndIf}
SectionEnd

Section -Post
  ${If} $PortableMode == "0"
    WriteUninstaller "$INSTDIR\uninst.exe"
    !ifndef _CPU_X64
    WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\dxglcfg.exe"
    !endif
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\dxglcfg.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  ${EndIf}
SectionEnd

Function PortableModeLeave
	${NSD_GetState} $RadioButtonPortable $0
	${If} $0 == ${BST_CHECKED}
		StrCpy $PortableMode "1"
	${Else}
		${NSD_GetState} $RadioButtonNormal $0
		${If} $0 == ${BST_CHECKED}
			StrCpy $PortableMode "0"
		${EndIf}
	${EndIf}
	${If} $PortableMode == "0"
		!ifdef _CPU_X64
		StrCpy $INSTDIR "$PROGRAMFILES64\DXGL"
		!else
		StrCpy $INSTDIR "$PROGRAMFILES\DXGL"
		!endif
		!ifndef _DEBUG
		SectionGetFlags ${SEC_VCREDIST} $0
		IntOp $0 $0 & 0x6F
		SectionGetFlags ${SEC_VCREDIST} $0
		!endif
		SectionGetFlags ${SEC_WINEDLLOVERRIDE} $0
		IntOp $0 $0 & 0x6F
		SectionSetFlags ${SEC_WINEDLLOVERRIDE} $0
		SectionGetFlags ${SEC_COMFIX} $0
		IntOp $0 $0 & 0x6F
		SectionSetFlags ${SEC_COMFIX} $0
	${Else}
		StrCpy $INSTDIR "$EXEDIR\DXGL"
		!ifndef _DEBUG
		SectionGetFlags ${SEC_VCREDIST} $0
		IntOp $0 $0 | 0x10
		SectionGetFlags ${SEC_VCREDIST} $0
		!endif
		SectionGetFlags ${SEC_WINEDLLOVERRIDE} $0
		IntOp $0 $0 | 0x10
		SectionSetFlags ${SEC_WINEDLLOVERRIDE} $0
		SectionGetFlags ${SEC_COMFIX} $0
		IntOp $0 $0 | 0x10
		SectionSetFlags ${SEC_COMFIX} $0
	${EndIf}
FunctionEnd

Function .onInit
  StrCpy $PortableMode "0"
  !ifdef _CPU_X64
  ${IfNot} ${RunningX64}
    MessageBox MB_OK|MB_ICONSTOP "This version of DXGL requires an x64 version of Windows."
	Quit
  ${EndIf}
  !endif
  !if ${COMPILER} == "VC2022_14"
  dxgl-nsis::CheckSSE2 $0
  Pop $0
  ${If} $0 == "0"
    MessageBox MB_OK|MB_ICONSTOP "This version of DXGL requires a processor with SSE2 capability.$\r\
	Please download the legacy build to use DXGL on your system."
	Quit
  ${EndIf}
  ${IfNot} ${AtLeastWin7}
    MessageBox MB_OK|MB_ICONSTOP "This version of DXGL requires at least Windows 7 Service Pack 1.$\r\
	If you need to run DXGL on Windows XP, XP x64, or Server 2003, please download the legacy build."
	Quit
  ${EndIf}
  ${If} ${IsWin7}
  ${AndIfNot} ${AtLeastServicePack} 1
    MessageBox MB_OK|MB_ICONSTOP "Your copy of Windows 7 or Windows Server 2008 R2 must be upgraded to Service Pack 1 before you can use this version of DXGL.$\r\
	Please visit https://support.microsoft.com/en-us/kb/976932/ for instructions on upgrading to Service Pack 1."
	Quit
  ${endif}
  !else if ${COMPILER} == "VC2010"
  ${IfNot} ${AtleastWinXP}
    MessageBox MB_OK|MB_ICONSTOP "This version of DXGL requires at least Windows XP Service Pack 3."
	Quit
  ${EndIf}
  ${If} ${IsWinXP}
  ${AndIfNot} ${AtLeastServicePack} 3
  ${AndIfNot} ${RunningX64}
    MessageBox MB_OK|MB_ICONSTOP "Your copy of Windows XP must be upgraded to Service Pack 3 before you can use DXGL.$\r\
	Please visit http://web.archive.org/web/20151010042325/https://support.microsoft.com/en-us/kb/322389/ for instructions on upgrading to Service Pack 3."
	Quit
  ${EndIf}
  ${If} ${IsWin2003}
  ${AndIfNot} ${AtLeastServicePack} 1
    MessageBox MB_OK|MB_ICONSTOP "Your copy of Windows Server 2003 must be upgraded to at least Service Pack 1 before you can use DXGL.$\r\
	Please visit http://web.archive.org/web/20150501080245/https://support.microsoft.com/en-us/kb/889100/ for instructions on upgrading to Service Pack 2."
	Quit
  ${EndIf}
  !else if ${COMPILER} == "VC2008"
  ${IfNot} ${AtleastWin2000}
    MessageBox MB_OK|MB_ICONEXCLAMATION "This version of DXGL requires at least Windows 2000."
    Quit
  ${EndIf}
  !else
  ${IfNot} ${AtleastWin2000}
    MessageBox MB_OK|MB_ICONEXCLAMATION "This version of DXGL requires at least Windows 2000.  You may attempt to install this build anyway however it is not guaranteed to run."
  ${EndIf}
  !endif
  !ifdef _DEBUG
  MessageBox MB_OK|MB_ICONEXCLAMATION "This is a debug build of DXGL.  It is not meant for regular \
  usage and requires the debug version of the Visual C++ runtime to work.$\r$\r\
  The debug runtime for Visual C++ is non-redistributable, as are executable files generated by compiling \
  in debug mode.  For more information please visit https://docs.microsoft.com/en-us/cpp/windows/preparing-a-test-machine-to-run-a-debug-executable$\r\
  If you downloaded this file, please immediately report this to admin@dxgl.org"
  !else
  !if ${download_runtime} >= 1
  !ifdef _CPU_X64
  !if ${COMPILER} != "VC2010"
  SetRegView 64
  !endif
  !endif
  ReadRegDWORD $0 HKLM ${runtime_regkey} ${runtime_regvalue}
  !ifdef _CPU_X64
  SetRegView 32
  !endif
  !if ${COMPILER} == "VC2022_14"
  StrCmp $0 1 skipvcredist1
  goto vcinstall
  skipvcredist1:
  !ifdef _CPU_X64
  !if ${COMPILER} != "VC2010"
  SetRegView 64
  !endif
  !endif
  ReadRegDWORD $0 HKLM ${runtime_regkey} ${runtime_regvalue2}
  !ifdef _CPU_X64
  SetRegView 32
  !endif
  ${VersionCompare} "$0" "14.44.35112" $1
  ${If} $1 == 0
    goto skipvcredist
  ${EndIf}
  ${If} $1 == 1
    goto skipvcredist
  ${EndIf}
  goto vcinstall
  !else
  StrCmp $0 1 skipvcredist
  goto vcinstall
  !endif
  skipvcredist:
  !ifndef _DEBUG
  SectionSetFlags ${SEC_VCREDIST} 0
  SectionSetText ${SEC_VCREDIST} ""
  !endif
  vcinstall:
  !endif
  !endif
  
  System::Call "${GetVersion} () .r0"
  IntOp $1 $0 & 255
  IntOp $2 $0 >> 8
  IntOp $2 $2 & 255
  IntCmp $1 6 CheckMinor BelowEight EightOrAbove
  CheckMinor:
  IntCmp $2 2 EightOrAbove BelowEight EightOrAbove
  EightOrAbove:
  SectionSetText ${SEC_COMFIX} "Fix DDraw COM registration (recommended)"
  goto VersionFinish
  BelowEight:
  SectionSetFlags ${SEC_COMFIX} 0
  VersionFinish:
  dxgl-nsis::IsWine $0
  Pop $0
  ${If} $0 == "0"
    SectionSetFlags ${SEC_WINEDLLOVERRIDE} 0
	SectionSetText ${SEC_WINEDLLOVERRIDE} ""
  ${EndIf}
FunctionEnd

LangString DESC_SEC01 ${LANG_ENGLISH} "Installs the required components for DXGL."
LangString DESC_SEC_VCREDIST ${LANG_ENGLISH} "Required MSVC redistributable was not detected.  Select to download the required redistributable from dxgl.org."
LangString DESC_SEC_WINEDLLOVERRIDE ${LANG_ENGLISH} "Sets a DLL override in Wine to allow DXGL to be used."
LangString DESC_SEC_COMFIX ${LANG_ENGLISH} "Adds a workaround for Windows 8 and above for COM initialization.  Applies to current user account."
LangString DESC_SEC_DEBUGSYMBOLS ${LANG_ENGLISH} "Copy PDB debug symbols to the installation directory."
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${SEC01} $(DESC_SEC01)
!ifndef _DEBUG
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_VCREDIST} $(DESC_SEC_VCREDIST)
!endif
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_WINEDLLOVERRIDE} $(DESC_SEC_WINEDLLOVERRIDE)
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_COMFIX} $(DESC_SEC_COMFIX)
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_DEBUGSYMBOLS} $(DESC_SEC_DEBUGSYMBOLS)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  MessageBox MB_YESNO "Do you want to remove all application profiles?" IDYES wipeprofile IDNO nowipeprofile
  wipeprofile:
  ExecWait '"$INSTDIR\dxglcfg.exe" uninstall 1'
  goto finishuninstall
  nowipeprofile:
  ExecWait '"$INSTDIR\dxglcfg.exe" uninstall 0'
  finishuninstall:
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\COPYING.txt"
  Delete "$INSTDIR\ThirdParty.txt"
  Delete "$INSTDIR\ReadMe.txt"
  Delete "$INSTDIR\ddraw.dll"
  Delete "$INSTDIR\dxglcfg.exe"
  Delete "$INSTDIR\dxgltest.exe"
  Delete "$INSTDIR\dxgl.chm"
  Delete "$INSTDIR\dxgl-example.ini"

  Delete "$SMPROGRAMS\${SMDIR}\Uninstall.lnk"
  Delete "$SMPROGRAMS\${SMDIR}\Website.lnk"
  Delete "$SMPROGRAMS\${SMDIR}\Configure DXGL.lnk"
  Delete "$SMPROGRAMS\${SMDIR}\Configure DXGL (x64).lnk"
  Delete "$SMPROGRAMS\${SMDIR}\DXGL Test.lnk"
  Delete "$SMPROGRAMS\${SMDIR}\Third-party Credits.lnk"
  Delete "$SMPROGRAMS\${SMDIR}\Example configuration file.lnk"

  RMDir "$SMPROGRAMS\${SMDIR}"
  RMDir "$INSTDIR"

  ReadRegDWORD $0 HKLM "Software\DXGL" "WineDLLOverride"
  ${If} $0 == "1"
    DeleteRegValue HKCU "Software\Wine\DllOverrides" "ddraw"
  ${EndIf}

  ReadRegDWORD $1 HKLM "Software\DXGL" "COMFix"
  ${If} $1 == "1"
    SetRegView 32
    DeleteRegKey HKCU "Software\Classes\CLSID\{D7B70EE0-4340-11CF-B063-0020AFC2CD35}"
    DeleteRegKey HKCU "Software\Classes\CLSID\{3C305196-50DB-11D3-9CFE-00C04FD930C5}"
	DeleteRegKey HKCU "Software\Classes\CLSID\{593817A0-7DB3-11CF-A2DE-00AA00B93356}"
  ${EndIf}

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  !ifndef _CPU_X64
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  !endif
  DeleteRegKey HKLM "Software\DXGL"
  SetAutoClose true
SectionEnd

!if ${SIGNTOOL} == 1
!if ${COMPILER} == "VC2022_14"
!finalize 'signtool sign /tr http://timestamp.sectigo.com /td sha384 /fd sha384 /as %1'
!endif
!endif
