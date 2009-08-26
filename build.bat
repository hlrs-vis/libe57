set CFLAGS=/EHsc /FD /MDd /Od /W3 /ZI -I\xerces-c-3.0.0\include
set LIBS=/link /DEBUG /LIBPATH:\xerces-c-3.0.0\lib xerces-c_3.lib

cl /c %CFLAGS% E57Foundation.cpp
cl /c %CFLAGS% E57FoundationImpl.cpp
cl %CFLAGS% DemoWrite01.cpp E57Foundation.obj E57FoundationImpl.obj %LIBS%
cl %CFLAGS% DemoRead01.cpp E57Foundation.obj E57FoundationImpl.obj %LIBS%
