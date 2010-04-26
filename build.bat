::
:: Build file for Microsoft Visual C++ version of E57 software package
:: Builds Reference Implementation, tools, doc examples, Foundation API documentation
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
:xerces_done

:: If an environment variable BOOST is set, use it.
@if T_%BOOST% NEQ T_ goto boost_done
:: try to find a boost installation.
@for /D %%f in (./boost*) do set BOOST=%%f
@if T_%BOOST% EQU T_ goto boost_fail
:boost_done

:: Get Subversion revision identifier of current directory into environment variable: RefImplRevisionId
for /f "delims=" %%a in ('svnversion %E57ROOT%') do @set RefImplRevisionId=%%a
@if [%RefImplRevisionId%] EQU [] set RefImplRevisionId=unknown
@echo Subversion revision id is %RefImplRevisionId%

:: Set current major/minor version number of Reference Implementation
@set RefImplMajor=0
@set RefImplMinor=1

:: Form the library name of Reference Implementation (e.g. "RefImpl-0-1-27.lib")
@set RefImplLib=RefImpl-%RefImplMajor%-%RefImplMinor%-%RefImplRevisionId%.lib

:: Set compiler/linker options
@set CFLAGS=/Od /D _DEBUG /D _CONSOLE /D WIN32 /D _WINDOWS /D XERCES_STATIC_LIBRARY /D _VC80_UPGRADE=0x0710 /FD /EHsc /MTd /W3 /ZI -I%E57ROOT%\include -I%E57ROOT%\include\time_conversion -I%E57ROOT%\src\refimpl -I%XERCES%\include -I%BOOST% /nologo /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE /D E57_REFIMPL_REVISION_ID=%RefImplRevisionId%
@set LIBS=%E57ROOT%\lib\%RefImplLib% %E57ROOT%\lib\LASReader.obj %E57ROOT%\lib\time_conversion.obj
@set LFLAGS=/link /DEBUG /SUBSYSTEM:CONSOLE /DYNAMICBASE:NO /LIBPATH:%XERCES%\lib xerces-c_static_3.lib /NODEFAULTLIB:libcmt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib

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

:: Compile and run API doc examples
cd %E57ROOT%\doc\FoundationAPI\examples
cl %CFLAGS% CheckInvariant.cpp %LIBS% %LFLAGS%
CheckInvariant >CheckInvariant.out
%E57ROOT%\bin\e57xmldump temp._e57 >CheckInvariant.xml
cl %CFLAGS% HelloWorld.cpp %LIBS% %LFLAGS%
HelloWorld >HelloWorld.out
%E57ROOT%\bin\e57xmldump temp._e57 >HelloWorld.xml
cl %CFLAGS% Cancel.cpp %LIBS% %LFLAGS% 
Cancel >Cancel.out
%E57ROOT%\bin\e57xmldump temp._e57 >Cancel.xml
cl %CFLAGS% Extensions.cpp %LIBS% %LFLAGS% 
Extensions >Extensions.out
%E57ROOT%\bin\e57xmldump temp._e57 >Extensions.xml
cl %CFLAGS% NameParse.cpp %LIBS% %LFLAGS% 
NameParse >NameParse.out
%E57ROOT%\bin\e57xmldump temp._e57 >NameParse.xml
cl %CFLAGS% ImageFileDump.cpp %LIBS% %LFLAGS% 
ImageFileDump >ImageFileDump.out
%E57ROOT%\bin\e57xmldump temp._e57 >ImageFileDump.xml
cl %CFLAGS% NodeFunctions.cpp %LIBS% %LFLAGS% 
NodeFunctions >NodeFunctions.out
%E57ROOT%\bin\e57xmldump temp._e57 >NodeFunctions.xml
cl %CFLAGS% StructureCreate.cpp %LIBS% %LFLAGS% 
StructureCreate >StructureCreate.out
%E57ROOT%\bin\e57xmldump temp._e57 >StructureCreate.xml
cl %CFLAGS% VectorCreate.cpp %LIBS% %LFLAGS% 
VectorCreate >VectorCreate.out
%E57ROOT%\bin\e57xmldump temp._e57 >VectorCreate.xml
cl %CFLAGS% VectorFunctions.cpp %LIBS% %LFLAGS% 
VectorFunctions >VectorFunctions.out
%E57ROOT%\bin\e57xmldump temp._e57 >VectorFunctions.xml
cl %CFLAGS% IntegerCreate.cpp %LIBS% %LFLAGS% 
IntegerCreate >IntegerCreate.out
%E57ROOT%\bin\e57xmldump temp._e57 >IntegerCreate.xml
cl %CFLAGS% ScaledIntegerCreate.cpp %LIBS% %LFLAGS% 
ScaledIntegerCreate >ScaledIntegerCreate.out
%E57ROOT%\bin\e57xmldump temp._e57 >ScaledIntegerCreate.xml
cl %CFLAGS% FloatCreate.cpp %LIBS% %LFLAGS% 
FloatCreate >FloatCreate.out
%E57ROOT%\bin\e57xmldump temp._e57 >FloatCreate.xml
cl %CFLAGS% StringCreate.cpp %LIBS% %LFLAGS% 
StringCreate >StringCreate.out
%E57ROOT%\bin\e57xmldump temp._e57 >StringCreate.xml
cl %CFLAGS% BlobCreate.cpp %LIBS% %LFLAGS% 
BlobCreate >BlobCreate.out
%E57ROOT%\bin\e57xmldump temp._e57 >BlobCreate.xml
cl %CFLAGS% CompressedVectorCreate.cpp %LIBS% %LFLAGS% 
CompressedVectorCreate >CompressedVectorCreate.out
%E57ROOT%\bin\e57xmldump temp._e57 >CompressedVectorCreate.xml
cl %CFLAGS% SourceDestBufferNumericCreate.cpp %LIBS% %LFLAGS% 
SourceDestBufferNumericCreate >SourceDestBufferNumericCreate.out
%E57ROOT%\bin\e57xmldump temp._e57 >SourceDestBufferNumericCreate.xml
cl %CFLAGS% SourceDestBufferStringCreate.cpp %LIBS% %LFLAGS% 
SourceDestBufferStringCreate >SourceDestBufferStringCreate.out
%E57ROOT%\bin\e57xmldump temp._e57 >SourceDestBufferStringCreate.xml
cl %CFLAGS% SourceDestBufferFunctions.cpp %LIBS% %LFLAGS% 
SourceDestBufferFunctions >SourceDestBufferFunctions.out
%E57ROOT%\bin\e57xmldump temp._e57 >SourceDestBufferFunctions.xml
cl %CFLAGS% E57ExceptionFunctions.cpp %LIBS% %LFLAGS% 
E57ExceptionFunctions >E57ExceptionFunctions.out
%E57ROOT%\bin\e57xmldump temp._e57 >E57ExceptionFunctions.xml
cl %CFLAGS% RawXML.cpp %LIBS% %LFLAGS% 
RawXML >RawXML.out
%E57ROOT%\bin\e57xmldump temp._e57 >RawXML.xml
cl %CFLAGS% Versions.cpp %LIBS% %LFLAGS% 
Versions >Versions.out
%E57ROOT%\bin\e57xmldump temp._e57 >Versions.xml

:: Create API html documenation
cd %E57ROOT%\doc\FoundationAPI
doxygen Doxyfile

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
