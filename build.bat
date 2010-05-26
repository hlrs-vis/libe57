::
:: Build file for Microsoft Visual C++ version of libe57 software package.
:: Builds Reference Implementation, tools.
::

:: Get current directory into environment variable E57ROOT
:: We assume that this batch file is in top-level directory of the distribution tree.
@for /f "delims=" %%a in ('cd') do @set E57ROOT=%%a
@echo Top-level of distribution tree is %E57ROOT%

:: If an environment variable XERCES is set, use it.
@if T_%XERCES% NEQ T_ goto xerces_done
:: try to find a xerces installation.
@for /D %%f in (./xerces*) do set XERCES=%%f
@if T_%XERCES% EQU T_ goto xerces_fail
@set XERCES=%E57ROOT%\%XERCES%
:xerces_done

:: If an environment variable BOOST is set, use it.
@if T_%BOOST% NEQ T_ goto boost_done
:: try to find a boost installation.
@for /D %%f in (./boost*) do set BOOST=%%f
@if T_%BOOST% EQU T_ goto boost_fail
:boost_done

:: Get Subversion revision identifier of current directory into environment variable: RefImplRevisionId
@for /f "delims=" %%a in ('svnversion %E57ROOT%') do @set RefImplRevisionId=%%a
@if [%RefImplRevisionId%] EQU [] set RefImplRevisionId=unknown
@echo Subversion revision id is %RefImplRevisionId%

:: Set current major/minor version number of Reference Implementation
@set RefImplMajor=0
@set RefImplMinor=1

:: Form the library name of Reference Implementation (e.g. "RefImpl-0-1-27.lib")
@set RefImplLib=RefImpl-%RefImplMajor%-%RefImplMinor%-%RefImplRevisionId%.lib

:: Set compiler/linker options
@set CFLAGS=/O2 /EHsc /MTd /W3 /nologo /D _DEBUG /D _CONSOLE /D WIN32 /D _WINDOWS /D XERCES_STATIC_LIBRARY /D _VC80_UPGRADE=0x0710 -I%E57ROOT%\include -I%E57ROOT%\include\time_conversion -I%E57ROOT%\src\refimpl -I%XERCES%\include -I%BOOST% /D E57_REFIMPL_REVISION_ID=%RefImplRevisionId%
@set LIBS=%E57ROOT%\lib\%RefImplLib% %E57ROOT%\lib\LASReader.obj %E57ROOT%\lib\time_conversion.obj
@set LFLAGS=/link /DEBUG /nologo %XERCES%\lib\xerces-c_static_3.lib advapi32.lib /NODEFAULTLIB:libcmt.lib

:: Build E57 Refrence Implementation library (static link)
cd %E57ROOT%\src\refimpl
cl /c %CFLAGS% E57Foundation.cpp E57FoundationImpl.cpp
lib /nologo /out:%E57ROOT%\lib\%RefImplLib% E57Foundation.obj E57FoundationImpl.obj

:: Build LAS format reader library
cd %E57ROOT%\src\LASReader
cl /c %CFLAGS% LASReader.cpp        /Fo%E57ROOT%\lib\LASReader.obj

:: Build time conversion library
cd %E57ROOT%\src\time_conversion
cl /c %CFLAGS% time_conversion.c    /Fo%E57ROOT%\lib\time_conversion.obj

:: Compile and link examples
cd %E57ROOT%\src\examples
cl %CFLAGS% DemoWrite01.cpp %LIBS% %LFLAGS%
cl %CFLAGS% DemoRead01.cpp  %LIBS% %LFLAGS%

:: Compile and link tools
cd %E57ROOT%\src\tools
cl %CFLAGS% las2e57.cpp     /Fe%E57ROOT%\bin\las2e57.exe    %LIBS% %LFLAGS%
cl %CFLAGS% e57fields.cpp   /Fe%E57ROOT%\bin\e57fields.exe  %LIBS% %LFLAGS%
cl %CFLAGS% e57xmldump.cpp  /Fe%E57ROOT%\bin\e57xmldump.exe %LIBS% %LFLAGS%

:: Return to where we started
cd %E57ROOT%

@goto end

:xerces_fail
@echo.
@echo Please download and install xerces from
@echo http://xerces.apache.org/xerces-c/download.cgi
@echo into a path in this directory first!
@echo Or set the XERCES environment variable to location of xerces installation.
@echo.
@echo e.g. into %~dp0xerces-c-3.0.1-x86-windows-vc-9.0 if using the 32bit VC9.0
@echo.
@goto end

:boost_fail
@echo.
@echo Please download and install boost from
@echo http://www.boost.org/users/download/
@echo into a path in this directory first!
@echo Or set the BOOST environment variable to location of boost installation.
@echo.
@echo e.g. into %~dp0boost_1_42_0
@echo.
@goto end

:end
