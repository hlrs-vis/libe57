@rem If an environment variable XERCES is set, use it.
@if T_%XERCES% NEQ T_ goto compile
@rem try to find a xerces installation.
@for /D %%f in (./xerces*) do set XERCES=%%f
@if T_%XERCES% EQU T_ goto xercesfail
:compile
@set CFLAGS=/Od /D "_DEBUG" /D "_CONSOLE" /D "WIN32" /D "_WINDOWS" /D "XERCES_STATIC_LIBRARY" /D "_VC80_UPGRADE=0x0710" /FD /EHsc /MTd /W3 /ZI -I%XERCES%\include
@set LIBS=E57Foundation.obj E57FoundationImpl.obj lasReader.obj time_conversion.obj /link /DEBUG /SUBSYSTEM:CONSOLE /DYNAMICBASE:NO /LIBPATH:%XERCES%\lib xerces-c_static_3.lib /NODEFAULTLIB:libcmt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib
cl /c %CFLAGS% E57Foundation.cpp
cl /c %CFLAGS% E57FoundationImpl.cpp
cl /c %CFLAGS% lasReader.cpp
cl /c %CFLAGS% time_conversion.c
cl %CFLAGS% DemoWrite01.cpp %LIBS%
cl %CFLAGS% DemoRead01.cpp %LIBS%
cl %CFLAGS% las2e57.cpp %LIBS%
cl %CFLAGS% e57fields.cpp %LIBS%
@goto end
:xercesfail
@echo.
@echo Please download and install xerces from
@echo http://xerces.apache.org/xerces-c/download.cgi
@echo into a path in this directory first!
@echo.
@echo e.g. into %~dp0xerces-c-3.0.1-x86-windows-vc-9.0 if using the 32bit VC9.0
@echo.
:end


