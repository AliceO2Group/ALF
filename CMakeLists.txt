# Copyright 2019-2020 CERN and copyright holders of ALICE O2.
# See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
# All rights not expressly granted are reserved.
#
# This software is distributed under the terms of the GNU General Public
# License v3 (GPL Version 3), copied verbatim in the file "COPYING".
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization
# or submit itself to any jurisdiction.

####################################
# General project definition
####################################

CMAKE_MINIMUM_REQUIRED(VERSION 3.5.2 FATAL_ERROR)
set(BUILD_SHARED_LIBS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

# Set cmake policy by version: https://cmake.org/cmake/help/latest/manual/cmake-policies.7.html
if(${CMAKE_VERSION} VERSION_LESS 3.12)
  cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
  cmake_policy(VERSION 3.12)
endif()

# Define project
project(ALF
  VERSION 0.0.1
  DESCRIPTION "O2 ALF"
  LANGUAGES CXX
)

# Documentation dir
add_subdirectory(doc)

# Add compiler flags for warnings and debug symbols
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")

# Set fPIC for all targets
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set CMAKE_INSTALL_LIBDIR explicitly to lib (to avoid lib64 on CC7)
set(CMAKE_INSTALL_LIBDIR lib)

# Used only for debugging
set(CMAKE_BUILD_TYPE "Debug")

# Set the default build type to "RelWithDebInfo"
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo"
    CACHE
    STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel Coverage."
    FORCE
  )
endif()


####################################
# Dependencies
####################################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Boost 1.56 REQUIRED)
find_package(Common REQUIRED)
find_package(ReadoutCard REQUIRED)
find_package(LLA REQUIRED)
find_package(DIM REQUIRED)
find_package(DimRpcParallel REQUIRED)

if(NOT APPLE)
  find_package(Python3 3.6 COMPONENTS Interpreter Development)
  if(Python3_FOUND)
    set(boost_python_component "python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}")
  else()
    # Backwards compatible. Can be removed once the Python3 recipe is stable
    message(WARNING "Python 3 was not found: falling back to Python 2")
    find_package(Python2 2.7 COMPONENTS Development REQUIRED)
    set(boost_python_component "python27")
  endif()
endif()

####################################
# Module, library and executable definition
####################################

add_library(ALF SHARED
  src/Ic.cxx
  src/Lla.cxx
  src/Sca.cxx
  src/ScaMftPsu.cxx
  src/ScBase.cxx
  src/Swt.cxx
  src/SwtWord.cxx
)

target_sources(ALF PRIVATE
  src/AlfServer.cxx
  src/DimServices/DimServices.cxx
  src/DimServices/ServiceNames.cxx
  $<$<BOOL:${Python2_FOUND}>:src/PythonInterface.cxx>
  $<$<BOOL:${Python3_FOUND}>:src/PythonInterface.cxx>
)

target_include_directories(ALF
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set(LINK_LIBS
    o2::LLA
    AliceO2::ReadoutCard
    AliceO2::Common
    Boost::boost
    $<$<BOOL:${Python2_FOUND}>:Boost::python27>
    $<$<BOOL:${Python2_FOUND}>:Python2::Python>
    $<$<BOOL:${Python3_FOUND}>:Boost::python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}>
    $<$<BOOL:${Python3_FOUND}>:Python3::Python>
)

# Link targets
target_link_libraries(ALF
  PUBLIC
    ${LINK_LIBS}
  PRIVATE
    dim::dim
    DimRpcParallel 
)

set_target_properties(ALF
  PROPERTIES
  OUTPUT_NAME
    O2Alf # Adhere to O2 naming conventions
)

# Use C++17
target_compile_features(ALF PUBLIC cxx_std_17)


####################################
# Executables
####################################
set (EXE_SRCS
  Alf.cxx
  )

set (EXE_NAMES
  o2-alf
  )

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  list(APPEND EXE_SRCS
    AlfClient.cxx
    AlfLibClient.cxx
    )

  list(APPEND EXE_NAMES
    o2-alf-client
    o2-alf-lib-client
    )
endif()

list(LENGTH EXE_SRCS count)
math(EXPR count "${count}-1")
foreach(i RANGE ${count})
  list(GET EXE_SRCS ${i} src)
  list(GET EXE_NAMES ${i} name)
  add_executable(${name} apps/${src})
  target_include_directories(${name}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )
  target_link_libraries(${name}
    PRIVATE
      ALF
      dim::dim
      DimRpcParallel
  )
endforeach()


####################################
# Install
####################################

include(GNUInstallDirs)

# Build targets with install rpath on Mac to dramatically speed up installation
# https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/RPATH-handling
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}" isSystemDir)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR}")
  endif()
  set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()
unset(isSystemDir)

# Install library and executables
install(TARGETS ALF ${EXE_NAMES}
  EXPORT ALFTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Create version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/cmake/ALFConfigVersion.cmake"
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY AnyNewerVersion
)

# Install headers # NO PUBLIC HEADERS
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/Alf DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

# Export targets
install(EXPORT ALFTargets
  FILE
    ALFTargets.cmake
  NAMESPACE
    o2::
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/ALF
)

# Configure and install Config files
configure_package_config_file(
  cmake/ALFConfig.cmake.in cmake/ALFConfig.cmake
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ALF"
  PATH_VARS CMAKE_INSTALL_PREFIX
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/ALFConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/ALFConfigVersion.cmake"
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/ALF
)
