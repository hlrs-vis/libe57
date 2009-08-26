@rem If an environment variable XERCES is set, use it.
@if T_%XERCES% NEQ T_ goto compile
@rem try to find a xerces installation.
@for /D %%f in (./xerces*) do set XERCES=%%f
@if T_%XERCES% EQU T_ goto xercesfail
@set CFLAGS=/EHsc /FD /MDd /Od /W3 /ZI -I%XERCES%\include
@set LIBS=/link /DEBUG /LIBPATH:%XERCES%\lib xerces-c_3.lib
:compile
cl /c %CFLAGS% E57Foundation.cpp
cl /c %CFLAGS% E57FoundationImpl.cpp
cl %CFLAGS% DemoWrite01.cpp E57Foundation.obj E57FoundationImpl.obj %LIBS%
cl %CFLAGS% DemoRead01.cpp E57Foundation.obj E57FoundationImpl.obj %LIBS%
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
