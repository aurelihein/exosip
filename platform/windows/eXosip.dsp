# Microsoft Developer Studio Project File - Name="eXosip" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=eXosip - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "eXosip.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "eXosip.mak" CFG="eXosip - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "eXosip - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "eXosip - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "eXosip - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".libs"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "EXOSIP_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\..\osip\include" /I "..\..\include" /D "NDEBUG" /D "EXOSIP_EXPORTS" /D "ENABLE_TRACE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OSIP_MT" /D "USE_TMP_BUFFER" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 msvcrt.lib osipparser2.lib osip2.lib Ws2_32.lib Iphlpapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /nodefaultlib /libpath:"..\..\..\osip\platform\windows\.libs"

!ELSEIF  "$(CFG)" == "eXosip - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".libs"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "EXOSIP_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\osip\include" /I "..\..\include" /D "EXOSIP_EXPORTS" /D "_DEBUG" /D "ENABLE_TRACE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OSIP_MT" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 msvcrtd.lib osipparser2.lib osip2.lib Ws2_32.lib Iphlpapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /nodefaultlib /pdbtype:sept /libpath:"..\..\..\osip\platform\windows\.libs"

!ENDIF 

# Begin Target

# Name "eXosip - Win32 Release"
# Name "eXosip - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\eXosip.c
# End Source File
# Begin Source File

SOURCE=.\eXosip.def
# End Source File
# Begin Source File

SOURCE=..\..\src\eXutils.c
# End Source File
# Begin Source File

SOURCE=..\..\src\jauth.c
# End Source File
# Begin Source File

SOURCE=..\..\src\jcall.c
# End Source File
# Begin Source File

SOURCE=..\..\src\jcallback.c
# End Source File
# Begin Source File

SOURCE=..\..\src\jdialog.c
# End Source File
# Begin Source File

SOURCE=..\..\src\jevents.c
# End Source File
# Begin Source File

SOURCE=..\..\src\jfreinds.c
# End Source File
# Begin Source File

SOURCE=..\..\src\jidentity.c
# End Source File
# Begin Source File

SOURCE=..\..\src\jnotify.c
# End Source File
# Begin Source File

SOURCE=..\..\src\jpipe.c
# End Source File
# Begin Source File

SOURCE=..\..\src\jreg.c
# End Source File
# Begin Source File

SOURCE=..\..\src\jrequest.c
# End Source File
# Begin Source File

SOURCE=..\..\src\jresponse.c
# End Source File
# Begin Source File

SOURCE=..\..\src\jsubscribe.c
# End Source File
# Begin Source File

SOURCE=..\..\src\jsubscribers.c
# End Source File
# Begin Source File

SOURCE=..\..\src\misc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\sdp_offans.c
# End Source File
# Begin Source File

SOURCE=..\..\src\udp.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\include\eXosip\eXosip.h
# End Source File
# Begin Source File

SOURCE=..\..\src\eXosip2.h
# End Source File
# Begin Source File

SOURCE=..\..\include\eXosip\eXosip_cfg.h
# End Source File
# Begin Source File

SOURCE=..\..\src\jpipe.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
