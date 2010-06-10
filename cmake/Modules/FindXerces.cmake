# $Id$

# Copyright 2010 Roland Schwarz, Riegl LMS GmbH
#
# Permission is hereby granted, free of charge, to any person or organization
# obtaining a copy of the software and accompanying documentation covered by
# this license (the "Software") to use, reproduce, display, distribute,
# execute, and transmit the Software, and to prepare derivative works of the
# Software, and to permit third-parties to whom the Software is furnished to
# do so, all subject to the following:
#
# The copyright notices in the Software and this entire statement, including
# the above license grant, this restriction and the following disclaimer,
# must be included in all copies of the Software, in whole or in part, and
# all derivative works of the Software, unless such copies or derivative
# works are solely in the form of machine-executable object code generated by
# a source language processor.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
# SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
# FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

if (Xerces_FIND_VERSION)
    message(WARNING "Finding a specific version of Xerces is not supported.")
endif (Xerces_FIND_VERSION)

find_path(Xerces_INCLUDE_DIR
    xercesc/sax2/SAX2XMLReader.hpp
    ${XERCES_ROOT}/include
)

if (WIN32)
if (Xerces_USE_STATIC_LIBS)
    find_library(Xerces_LIBRARY_DEBUG
        NAMES xerces-c_static_3D.lib libxerces-c.a
        PATHS ${XERCES_ROOT}/lib
    )

    find_library(Xerces_LIBRARY_RELEASE
        NAMES xerces-c_static_3.lib libxerces-c.a
        PATHS ${XERCES_ROOT}/lib
    )
else(Xerces_USE_STATIC_LIBS)
    find_library(Xerces_LIBRARY_DEBUG
        NAMES xerces-c_3D.lib libxerces-c.dll.a
        PATHS ${XERCES_ROOT}/lib
    )

    find_library(Xerces_LIBRARY_RELEASE
        NAMES xerces-c_3.lib libxerces-c.dll.a
        PATHS ${XERCES_ROOT}/lib
    )
endif(Xerces_USE_STATIC_LIBS)
endif(WIN32)

mark_as_advanced(
    Xerces_INCLUDE_DIR
    Xerces_LIBRARY_DEBUG
    Xerces_LIBRARY_RELEASE
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Xerces
    "Unable to find Xerces: \nPlease set the XERCES_ROOT variable to the Xerces directory.\n"
    Xerces_INCLUDE_DIR
    Xerces_LIBRARY_DEBUG
    Xerces_LIBRARY_RELEASE
)

set(Xerces_FOUND ${XERCES_FOUND})
