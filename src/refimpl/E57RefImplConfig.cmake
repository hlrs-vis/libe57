# $Id$
# This is the CMake project configuration file for using E57RefImpl.
#
# Copyright 2010-2011 Roland Schwarz, Riegl LMS GmbH
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

# == Using E57refImpl library: ==
#
#   find_package( E57RefImpl )
#   add_executable( my_app
#        my_app.cpp
#   )
#   target_link_libraries( my_app
#       ${E57RefImpl_SCANLIB_LIBRARIES}
#   )
#
#
# == Variables defined by this module: ==
#
# E57RefImpl_LIBRARY_DEBUG
# E57RefImpl_LIBRARY_RELEASE
# E57RefImpl_LIBRARY
#
# E57RefImpl_LIBRARIES
# E57RefImpl_INCLUDE_DIRS
# E57RefImpl_LIBRARY_DIRS
#
# NOTE: You will also need to include the boost and xerces libraries to your
# project.

SET(E57RefImpl_FOUND TRUE)

SET( E57RefImpl_LIBRARY "E57RefImpl_LIBRARY-NOTFOUND" )
SET( E57RefImpl_LIBRARY_RELEASE "E57RefImpl_LIBRARY_RELEASE-NOTFOUND" )
SET( E57RefImpl_LIBRARY_DEBUG "E57RefImpl_LIBRARY_DEBUG-NOTFOUND")

FIND_LIBRARY(E57RefImpl_LIBRARY_RELEASE
    NAMES   libE57RefImpl
            E57RefImpl
    HINTS  ${E57RefImpl_DIR}/lib
)

FIND_LIBRARY(E57RefImpl_LIBRARY_DEBUG
    NAMES   libE57RefImpl-d
            E57RefImpl-d
    HINTS  ${E57RefImpl_DIR}/lib
)

MARK_AS_ADVANCED(
    E57RefImpl_LIBRARY_RELEASE
    E57RefImpl_LIBRARY_DEBUG
    E57RefImpl_LIBRARY
)

IF (E57RefImpl_LIBRARY_DEBUG AND E57RefImpl_LIBRARY_RELEASE)
  # if the generator supports configuration types then set
  # optimized and debug libraries, or if the CMAKE_BUILD_TYPE has a value
  IF (CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
    SET(E57RefImpl_LIBRARY optimized ${E57RefImpl_LIBRARY_RELEASE} debug ${E57RefImpl_LIBRARY_DEBUG})
  ELSE()
    # if there are no configuration types and CMAKE_BUILD_TYPE has no value
    # then just use the release libraries
    SET(E57RefImpl_LIBRARY ${E57RefImpl_LIBRARY_RELEASE} )
  ENDIF()
  # FIXME: This probably should be set for both cases
  SET(E57RefImpl_LIBRARIES optimized ${E57RefImpl_LIBRARY_RELEASE} debug ${E57RefImpl_LIBRARY_DEBUG})
ENDIF()

# if only the release version was found, set the debug variable also to the release version
IF (E57RefImpl_LIBRARY_RELEASE AND NOT E57RefImpl_LIBRARY_DEBUG)
  SET(E57RefImpl_LIBRARY_DEBUG ${E57RefImpl_LIBRARY_RELEASE})
  SET(E57RefImpl_LIBRARY       ${E57RefImpl_LIBRARY_RELEASE})
  SET(E57RefImpl_LIBRARIES     ${E57RefImpl_LIBRARY_RELEASE})
ENDIF()

# if only the debug version was found, set the release variable also to the debug version
IF (E57RefImpl_LIBRARY_DEBUG AND NOT E57RefImpl_LIBRARY_RELEASE)
  SET(E57RefImpl_LIBRARY_RELEASE ${E57RefImpl_LIBRARY_DEBUG})
  SET(E57RefImpl_LIBRARY         ${E57RefImpl_LIBRARY_DEBUG})
  SET(E57RefImpl_LIBRARIES       ${E57RefImpl_LIBRARY_DEBUG})
ENDIF()

IF (E57RefImpl_LIBRARY)
    set(E57RefImpl_LIBRARY ${E57RefImpl_LIBRARY} CACHE FILEPATH "The E57RefImpl library")
    # Remove superfluous "debug" / "optimized" keywords from
    # RiVLib_LIBRARY_DIRS
    FOREACH(_E57RefImpl_my_lib ${E57RefImpl_LIBRARY})
        GET_FILENAME_COMPONENT(_E57RefImpl_my_lib_path "${_E57RefImpl_my_lib}" PATH)
        LIST(APPEND E57RefImpl_LIBRARY_DIRS ${_E57RefImpl_my_lib_path})
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES E57RefImpl_LIBRARY_DIRS)

    SET(E57RefImpl_LIBRARY_DIRS ${E57RefImpl_LIBRARY_DIRS} FILEPATH "E57RefImpl library directory")
    SET(E57RefImpl_FOUND ON CACHE INTERNAL "Whether the E57RefImpl component was found")
    SET(E57RefImpl_LIBRARIES ${E57RefImpl_LIBRARIES} ${E57RefImpl_LIBRARY})
ELSE(E57RefImpl_LIBRARY)
    SET(E57RefImpl_FOUND FALSE) #FIXME: doesn't get propagated to caller
ENDIF(E57RefImpl_LIBRARY)

IF (E57RefIml_FOUND)
    SET(E57RefImpl_INCLUDE_DIR ${E57RefIml_DIR}/include FILEPATH "E57RefImpl include directory")
    SET(E57RefImpl_INCLUDE_DIRS ${E57RefImpl_INCLUDE_DIR})
    SET(E57RefImpl_ROOT_DIR ${E57RefIml_DIR})
ENDIF()


