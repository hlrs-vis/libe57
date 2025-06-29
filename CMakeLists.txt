# $Id$
# This is the CMake project file for the libe57 reference implementation
#
# Copyright 2010-2012 Roland Schwarz, Riegl LMS GmbH
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
#
# Requirements:
#     Xerces library: http://xerces.apache.org/
#     Boost library: http://www.boost.org
# Notes:
#     Since there is not standard cmake module to find the xerces library
#     we provide one with this distribution. It should be able to find
#     the library from the XERCES_ROOT cmake variable. Standard layout, as
#     with the binary packages from apache is assumed.
# If you find any errors or have suggestion to improve the build script:
# patches are most welcome! Please send them to the development mailing list.


cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_STANDARD_REQUIRED 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_C_STANDARD 11)
set (CMAKE_C_STANDARD_REQUIRED 99)
set(CMAKE_C_EXTENSIONS OFF)

# Override flags to enable prepare for linking to static runtime
set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cxx_flag_overrides.cmake)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set a private module find path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

project(E57RefImpl)

# developer adjustable version numbers
set(${PROJECT_NAME}_MAJOR_VERSION 1)
set(${PROJECT_NAME}_MINOR_VERSION 1)

include(Tags)

# propose a default installation directory
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    string(REGEX REPLACE "/${PROJECT_NAME}" "" CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
    set(T_ ${PROJECT_NAME})
    set(T_ ${T_}-${${PROJECT_NAME}_MAJOR_VERSION})
    set(T_ ${T_}.${${PROJECT_NAME}_MINOR_VERSION})
    set(T_ ${T_}.${${PROJECT_NAME}_BUILD_VERSION})
    set(T_ ${T_}-${${PROJECT_NAME}_BUILD_TAG})
    set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/${T_}
        CACHE PATH
        "Install path prefix, prepended onto install directories."
        FORCE
    )
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

find_package(Threads REQUIRED)

# Find the Boost and Xerces libraries

set(Boost_USE_MULTITHREADED ON)
find_package(Boost
    COMPONENTS
        program_options
        system
        thread
        filesystem
)
if (NOT Boost_FOUND)
    set(BOOST_ROOT CACHE PATH  "Location of the boost root directory" )
    message(FATAL_ERROR
"Unable to find boost library.
Please set the BOOST_ROOT to point to the boost distribution files."
)
endif(NOT Boost_FOUND)

find_package(Xerces)
if (NOT Xerces_FOUND)
    set(XERCES_ROOT CACHE PATH "Location of the xerces library")
    message(FATAL_ERROR
"Unable to find xerces library.
Please set the the XERCES_ROOT to point to the root of the xerces directory."
)
endif (NOT Xerces_FOUND)

set(XML_LIBRARIES ${Xerces_LIBRARY})
set(XML_INCLUDE_DIRS ${Xerces_INCLUDE_DIR})

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    add_definitions(-DLINUX)
    find_package(ICU REQUIRED)
    set(XML_LIBRARIES ${XML_LIBRARIES} ${ICU_LIBRARIES})
    set(XML_INCLUDE_DIRS ${XML_INCLUDE_DIRS} ${ICU_INCLUDE_DIRS})
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_definitions(-DWINDOWS)
endif()

add_definitions(${Boost_LIB_DIAGNOSTIC_DEFINITIONS})
add_definitions(-DBOOST_ALL_NO_LIB -DXERCES_STATIC_LIBRARY)
add_definitions(-DE57_REFIMPL_REVISION_ID=${PROJECT_NAME}-${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_BUILD_VERSION}-${${PROJECT_NAME}_BUILD_TAG})

configure_file (
  "${PROJECT_SOURCE_DIR}/include/config.h.in"
  "${PROJECT_BINARY_DIR}/include/config.h"
)

include_directories(
    ${PROJECT_BINARY_DIR}/include
    include
    include/time_conversion
    src/refimpl
    ${XML_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIR}
)

link_directories(
    ${Boost_LIBRARY_DIRS}
)


#
# The reference implementation
#

add_library( E57RefImpl STATIC
    src/refimpl/E57Foundation.cpp
    src/refimpl/E57FoundationImpl.cpp
    src/refimpl/E57FoundationImpl.h
    include/E57Foundation.h
    src/refimpl/E57Simple.cpp
    src/refimpl/E57SimpleImpl.cpp
    src/time_conversion/time_conversion.c
    include/E57Simple.h
)
set_target_properties( E57RefImpl
    PROPERTIES DEBUG_POSTFIX "-d"
)

#
# Files for LAS format support
#

add_library( LASReader STATIC
    src/LASReader/LASReader.cpp
    include/LASReader.h
)

#
# Time conversion utilities
#

add_library( time_conversion STATIC
    src/time_conversion/time_conversion.c
    include/time_conversion/time_conversion.h
    include/time_conversion/basictypes.h
    include/time_conversion/constants.h
    include/time_conversion/gnss_error.h
)

#
# Example programs
#

add_executable( DemoWrite01
    src/examples/DemoWrite01.cpp
)
target_link_libraries( DemoWrite01
    E57RefImpl
    ${XML_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)
add_executable( DemoRead01
    src/examples/DemoRead01.cpp
)
target_link_libraries( DemoRead01
    E57RefImpl
    ${XML_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

#
# E57-4 Tools
#

add_executable( las2e57
    src/tools/las2e57.cpp
)
target_link_libraries( las2e57
    E57RefImpl
    LASReader
    time_conversion
    ${XML_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)
add_executable( e57fields
    src/tools/e57fields.cpp
)
target_link_libraries( e57fields
    E57RefImpl
    ${XML_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)
add_executable( e57xmldump
    src/tools/e57xmldump.cpp
)
target_link_libraries( e57xmldump
    E57RefImpl
    ${XML_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)
add_executable( e57validate
    src/tools/e57validate.cpp
)
target_link_libraries( e57validate
    E57RefImpl
    ${XML_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)
add_executable( e57unpack
    src/tools/e57unpack.cpp
)
target_link_libraries( e57unpack
    E57RefImpl
    ${XML_LIBRARIES}
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

#
# Install section
#

install(
    TARGETS
        E57RefImpl
        e57fields
        e57xmldump
        e57unpack
        e57validate
        las2e57
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
)
install(
    FILES
        include/E57Foundation.h
        include/E57Simple.h
    DESTINATION include/e57
)

install(
    FILES
        CHANGES.TXT
        README.TXT
    DESTINATION share/doc/libe57/
)

install(
    FILES
        src/refimpl/E57RefImplConfig.cmake
    DESTINATION lib/cmake/
)

#include (InstallRequiredSystemLibraries)

set (CPACK_PACKAGE_VENDOR "Riegl LMS GmbH")
set (CPACK_PACKAGE_VERSION_MAJOR "${${PROJECT_NAME}_MAJOR_VERSION}")
set (CPACK_PACKAGE_VERSION_MINOR "${${PROJECT_NAME}_MINOR_VERSION}")
set (CPACK_PACKAGE_VERSION_PATCH "${${PROJECT_NAME}_BUILD_VERSION}")
set (CPACK_SYSTEM_NAME "${${PROJECT_NAME}_BUILD_TAG}")
set (CPACK_GENERATOR "ZIP")
set (CPACK_STRIP_FILES "TRUE")

include (CPack)
