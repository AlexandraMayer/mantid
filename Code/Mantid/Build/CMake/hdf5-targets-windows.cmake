# Generated by CMake 2.8.8

IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 2.5)
   MESSAGE(FATAL_ERROR "CMake >= 2.6.0 required")
ENDIF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 2.5)
CMAKE_POLICY(PUSH)
CMAKE_POLICY(VERSION 2.6)
#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
SET(CMAKE_IMPORT_FILE_VERSION 1)

# Create imported target hdf5
ADD_LIBRARY(hdf5 SHARED IMPORTED)

# Create imported target hdf5_cpp
ADD_LIBRARY(hdf5_cpp SHARED IMPORTED)

# Create imported target hdf5_hl
ADD_LIBRARY(hdf5_hl SHARED IMPORTED)

# Create imported target hdf5_hl_cpp
ADD_LIBRARY(hdf5_hl_cpp SHARED IMPORTED)

# Import target "hdf5" for configuration "Debug"
# SET_PROPERTY(TARGET hdf5 APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
# SET_TARGET_PROPERTIES(hdf5 PROPERTIES
  # IMPORTED_IMPLIB_DEBUG "${CMAKE_LIBRARY_PATH}/hdf5ddll.lib"
  # IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "kernel32;ws2_32;wsock32"
  # IMPORTED_LOCATION_DEBUG "${CMAKE_LIBRARY_PATH}/hdf5ddll.dll"
  # )

# Import target "hdf5_cpp" for configuration "Debug"
# SET_PROPERTY(TARGET hdf5_cpp APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
# SET_TARGET_PROPERTIES(hdf5_cpp PROPERTIES
  # IMPORTED_IMPLIB_DEBUG "${CMAKE_LIBRARY_PATH}/hdf5_cppddll.lib"
  # IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "hdf5"
  # IMPORTED_LOCATION_DEBUG "${CMAKE_LIBRARY_PATH}/hdf5_cppddll.dll"
  # )

# Import target "hdf5_tools" for configuration "Debug"
# SET_PROPERTY(TARGET hdf5_tools APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
# SET_TARGET_PROPERTIES(hdf5_tools PROPERTIES
  # IMPORTED_IMPLIB_DEBUG "${CMAKE_LIBRARY_PATH}/hdf5_toolsddll.lib"
  # IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "hdf5"
  # IMPORTED_LOCATION_DEBUG "${CMAKE_LIBRARY_PATH}/hdf5_toolsddll.dll"
  # )

# Import target "hdf5_hl" for configuration "Debug"
# SET_PROPERTY(TARGET hdf5_hl APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
# SET_TARGET_PROPERTIES(hdf5_hl PROPERTIES
  # IMPORTED_IMPLIB_DEBUG "${CMAKE_LIBRARY_PATH}/hdf5_hlddll.lib"
  # IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "hdf5"
  # IMPORTED_LOCATION_DEBUG "${CMAKE_LIBRARY_PATH}/hdf5_hlddll.dll"
  # )

# Import target "hdf5_hl_cpp" for configuration "Debug"
# SET_PROPERTY(TARGET hdf5_hl_cpp APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
# SET_TARGET_PROPERTIES(hdf5_hl_cpp PROPERTIES
  # IMPORTED_IMPLIB_DEBUG "${CMAKE_LIBRARY_PATH}/hdf5_hl_cppddll.lib"
  # IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "hdf5_hl;hdf5"
  # IMPORTED_LOCATION_DEBUG "${CMAKE_LIBRARY_PATH}/hdf5_hl_cppddll.dll"
  # )

# Import target "hdf5" for configuration "Release"
SET_PROPERTY(TARGET hdf5 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(hdf5 PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${CMAKE_LIBRARY_PATH}/hdf5dll.lib"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "kernel32;ws2_32;wsock32"
  IMPORTED_LOCATION_RELEASE "${CMAKE_LIBRARY_PATH}/hdf5dll.dll"
  )

# Import target "hdf5_cpp" for configuration "Release"
SET_PROPERTY(TARGET hdf5_cpp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(hdf5_cpp PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${CMAKE_LIBRARY_PATH}/hdf5_cppdll.lib"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "hdf5"
  IMPORTED_LOCATION_RELEASE "${CMAKE_LIBRARY_PATH}/hdf5_cppdll.dll"
  )

# Import target "hdf5_hl" for configuration "Release"
SET_PROPERTY(TARGET hdf5_hl APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(hdf5_hl PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${CMAKE_LIBRARY_PATH}/hdf5_hldll.lib"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "hdf5"
  IMPORTED_LOCATION_RELEASE "${CMAKE_LIBRARY_PATH}/hdf5_hldll.dll"
  )

# Import target "hdf5_hl_cpp" for configuration "Release"
SET_PROPERTY(TARGET hdf5_hl_cpp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
SET_TARGET_PROPERTIES(hdf5_hl_cpp PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${CMAKE_LIBRARY_PATH}/hdf5_hl_cppdll.lib"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "hdf5_hl;hdf5"
  IMPORTED_LOCATION_RELEASE "${CMAKE_LIBRARY_PATH}/hdf5_hl_cppdll.dll"
  )

# Import target "hdf5" for configuration "MinSizeRel"
# SET_PROPERTY(TARGET hdf5 APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
# SET_TARGET_PROPERTIES(hdf5 PROPERTIES
  # IMPORTED_IMPLIB_MINSIZEREL "${CMAKE_LIBRARY_PATH}/hdf5dll.lib"
  # IMPORTED_LINK_INTERFACE_LIBRARIES_MINSIZEREL "kernel32;ws2_32;wsock32"
  # IMPORTED_LOCATION_MINSIZEREL "${CMAKE_LIBRARY_PATH}/hdf5dll.dll"
  # )

# Import target "hdf5_cpp" for configuration "MinSizeRel"
# SET_PROPERTY(TARGET hdf5_cpp APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
# SET_TARGET_PROPERTIES(hdf5_cpp PROPERTIES
  # IMPORTED_IMPLIB_MINSIZEREL "${CMAKE_LIBRARY_PATH}/hdf5_cppdll.lib"
  # IMPORTED_LINK_INTERFACE_LIBRARIES_MINSIZEREL "hdf5"
  # IMPORTED_LOCATION_MINSIZEREL "${CMAKE_LIBRARY_PATH}/hdf5_cppdll.dll"
  # )

# Import target "hdf5_tools" for configuration "MinSizeRel"
# SET_PROPERTY(TARGET hdf5_tools APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
# SET_TARGET_PROPERTIES(hdf5_tools PROPERTIES
  # IMPORTED_IMPLIB_MINSIZEREL "${CMAKE_LIBRARY_PATH}/hdf5_toolsdll.lib"
  # IMPORTED_LINK_INTERFACE_LIBRARIES_MINSIZEREL "hdf5"
  # IMPORTED_LOCATION_MINSIZEREL "${CMAKE_LIBRARY_PATH}/hdf5_toolsdll.dll"
  # )

# Import target "hdf5_hl" for configuration "MinSizeRel"
# SET_PROPERTY(TARGET hdf5_hl APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
# SET_TARGET_PROPERTIES(hdf5_hl PROPERTIES
  # IMPORTED_IMPLIB_MINSIZEREL "${CMAKE_LIBRARY_PATH}/hdf5_hldll.lib"
  # IMPORTED_LINK_INTERFACE_LIBRARIES_MINSIZEREL "hdf5"
  # IMPORTED_LOCATION_MINSIZEREL "${CMAKE_LIBRARY_PATH}/hdf5_hldll.dll"
  # )

# Import target "hdf5_hl_cpp" for configuration "MinSizeRel"
# SET_PROPERTY(TARGET hdf5_hl_cpp APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
# SET_TARGET_PROPERTIES(hdf5_hl_cpp PROPERTIES
  # IMPORTED_IMPLIB_MINSIZEREL "${CMAKE_LIBRARY_PATH}/hdf5_hl_cppdll.lib"
  # IMPORTED_LINK_INTERFACE_LIBRARIES_MINSIZEREL "hdf5_hl;hdf5"
  # IMPORTED_LOCATION_MINSIZEREL "${CMAKE_LIBRARY_PATH}/hdf5_hl_cppdll.dll"
  # )

# Import target "hdf5" for configuration "RelWithDebInfo"
# SET_PROPERTY(TARGET hdf5 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
# SET_TARGET_PROPERTIES(hdf5 PROPERTIES
  # IMPORTED_IMPLIB_RELWITHDEBINFO "${CMAKE_LIBRARY_PATH}/hdf5dll.lib"
  # IMPORTED_LINK_INTERFACE_LIBRARIES_RELWITHDEBINFO "kernel32;ws2_32;wsock32"
  # IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_LIBRARY_PATH}/hdf5dll.dll"
  # )

# Import target "hdf5_cpp" for configuration "RelWithDebInfo"
# SET_PROPERTY(TARGET hdf5_cpp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
# SET_TARGET_PROPERTIES(hdf5_cpp PROPERTIES
  # IMPORTED_IMPLIB_RELWITHDEBINFO "${CMAKE_LIBRARY_PATH}/hdf5_cppdll.lib"
  # IMPORTED_LINK_INTERFACE_LIBRARIES_RELWITHDEBINFO "hdf5"
  # IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_LIBRARY_PATH}/hdf5_cppdll.dll"
  # )

# Import target "hdf5_tools" for configuration "RelWithDebInfo"
# SET_PROPERTY(TARGET hdf5_tools APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
# SET_TARGET_PROPERTIES(hdf5_tools PROPERTIES
  # IMPORTED_IMPLIB_RELWITHDEBINFO "${CMAKE_LIBRARY_PATH}/hdf5_toolsdll.lib"
  # IMPORTED_LINK_INTERFACE_LIBRARIES_RELWITHDEBINFO "hdf5"
  # IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_LIBRARY_PATH}/hdf5_toolsdll.dll"
  # )

#Import target "hdf5_hl" for configuration "RelWithDebInfo"
# SET_PROPERTY(TARGET hdf5_hl APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
# SET_TARGET_PROPERTIES(hdf5_hl PROPERTIES
  # IMPORTED_IMPLIB_RELWITHDEBINFO "${CMAKE_LIBRARY_PATH}/hdf5_hldll.lib"
  # IMPORTED_LINK_INTERFACE_LIBRARIES_RELWITHDEBINFO "hdf5"
  # IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_LIBRARY_PATH}/hdf5_hldll.dll"
  # )

# Import target "hdf5_hl_cpp" for configuration "RelWithDebInfo"
# SET_PROPERTY(TARGET hdf5_hl_cpp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
# SET_TARGET_PROPERTIES(hdf5_hl_cpp PROPERTIES
  # IMPORTED_IMPLIB_RELWITHDEBINFO "${CMAKE_LIBRARY_PATH}/hdf5_hl_cppdll.lib"
  # IMPORTED_LINK_INTERFACE_LIBRARIES_RELWITHDEBINFO "hdf5_hl;hdf5"
  # IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_LIBRARY_PATH}/hdf5_hl_cppdll.dll"
  # )

# Commands beyond this point should not need to know the version.
SET(CMAKE_IMPORT_FILE_VERSION)
CMAKE_POLICY(POP)