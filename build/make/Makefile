VPATH = ../../src:../../examples
CPPFLAGS += -I../../include
CPPFLAGS += -I../../include/e57/detail
libe57.a: CPPFLAGS += -DTIXML_USE_STL

all: e57toasc asctoe57

libe57.a: libe57.a(tinyxml.o) libe57.a(tinyxmlparser.o) libe57.a(tinyxmlerror.o) 

e57toasc: e57toasc.cpp libe57.a

asctoe57: asctoe57.cpp libe57.a
