##########################################################################
# Does the CPack configuration for Windows/NSIS
#
# Bundles python
# Copies include directories
# Copies scons
# Copies files required for User Algorithms
# Copies selected third party dlls accross
# Sets up env variables, shortcuts and required folders post install and post uninstall.
###########################################################################

    # Windows CPACK specifics
    set( CPACK_GENERATOR "NSIS" )
    set( CPACK_INSTALL_PREFIX "/")
    set( CPACK_NSIS_DISPLAY_NAME "Mantid${CPACK_PACKAGE_SUFFIX}")
    set( CPACK_PACKAGE_NAME "Mantid${CPACK_PACKAGE_SUFFIX}" )
    set( CPACK_PACKAGE_INSTALL_DIRECTORY "MantidInstall${CPACK_PACKAGE_SUFFIX}") 
    set( CPACK_NSIS_INSTALL_ROOT "C:")
    set( CPACK_PACKAGE_EXECUTABLES "MantidPlot;MantidPlot")
    set( CPACK_NSIS_MENU_LINKS "bin\\\\MantidPlot.exe" "MantidPlot")
    
    set( CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/Images\\\\MantidPlot_Icon_32offset.png" )
    set( CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/Images\\\\MantidPlot_Icon_32offset.ico" )
    set( CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/Images\\\\MantidPlot_Icon_32offset.ico" )
    set( WINDOWS_DEPLOYMENT_TYPE "Release" CACHE STRING "Type of deployment used")
    set_property(CACHE WINDOWS_DEPLOYMENT_TYPE PROPERTY STRINGS Release Debug)
    mark_as_advanced(WINDOWS_DEPLOYMENT_TYPE)
    
    # Manually place necessary files and directories
    
    # Python bundle here.
    install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/Python27/DLLs DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
    install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/Python27/Lib DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE PATTERN "_d.pyd" EXCLUDE )
    install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/Python27/Scripts DESTINATION bin PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
    install ( FILES ${PY_DLL_PREFIX}${PY_DLL_SUFFIX_RELEASE} ${PYTHON_EXECUTABLE} ${PYTHONW_EXECUTABLE} DESTINATION bin )

    install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/qt_plugins/imageformats DESTINATION plugins/qtplugins PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
    install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/Installers/WinInstaller/qt.conf DESTINATION bin )
    
    # include files
    install ( DIRECTORY ${CMAKE_INCLUDE_PATH}/boost DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
    install ( DIRECTORY ${CMAKE_INCLUDE_PATH}/Poco DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
    install ( DIRECTORY ${CMAKE_INCLUDE_PATH}/nexus DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
    install ( FILES ${CMAKE_INCLUDE_PATH}/napi.h DESTINATION include )
    install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/Kernel/inc/MantidKernel DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
    install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/Geometry/inc/MantidGeometry DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
    install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/API/inc/MantidAPI DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
    
    # scons directory for sser building
    install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Installers/WinInstaller/scons-local/ DESTINATION scons-local PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
    # user algorithms
    install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/ DESTINATION UserAlgorithms FILES_MATCHING PATTERN "*.h" )
    install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/ DESTINATION UserAlgorithms FILES_MATCHING PATTERN "*.cpp" )
    install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/build.bat ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/createAlg.py 
              ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/SConstruct DESTINATION UserAlgorithms )
    install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidKernel.lib" DESTINATION UserAlgorithms)
    install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidGeometry.lib" DESTINATION UserAlgorithms)
    install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidAPI.lib" DESTINATION UserAlgorithms)
    install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidDataObjects.lib" DESTINATION UserAlgorithms)
    install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidCurveFitting.lib" DESTINATION UserAlgorithms)
    install ( FILES ${CMAKE_LIBRARY_PATH}/PocoFoundation.lib ${CMAKE_LIBRARY_PATH}/PocoXML.lib ${CMAKE_LIBRARY_PATH}/boost_date_time-vc100-mt-1_43.lib DESTINATION UserAlgorithms)
    
    # Copy MSVC runtime libraries
    install (FILES ${CMAKE_LIBRARY_PATH}/CRT/msvcp100.dll ${CMAKE_LIBRARY_PATH}/CRT/msvcr100.dll ${CMAKE_LIBRARY_PATH}/CRT/vcomp100.dll DESTINATION bin)
    # Copy Intel fortran libraries from numpy to general bin directory. Both numpy & scipy are compiled with intel compiler as it is the only way to get 64-bit libs at the moment.
    # This means scipy requires the intel libraries that are stuck in numpy/core.
    file ( GLOB INTEL_DLLS "${CMAKE_LIBRARY_PATH}/Python27/Lib/site-packages/numpy/core/*.dll" )
    install ( FILES ${INTEL_DLLS} DESTINATION ${INBUNDLE}bin )

    # Copy third party dlls excluding selected Qt ones and debug ones
    install ( DIRECTORY ${CMAKE_LIBRARY_PATH}/ DESTINATION bin FILES_MATCHING PATTERN "*.dll" 
    REGEX "${CMAKE_LIBRARY_PATH}/CRT/*" EXCLUDE 
    REGEX "${CMAKE_LIBRARY_PATH}/Python27/*" EXCLUDE 
    REGEX "${CMAKE_LIBRARY_PATH}/qt_plugins/*" EXCLUDE 
    REGEX "(QtDesigner4.dll)|(QtDesignerComponents4.dll)|(QtScript4.dll)|(-gd-)|(d4.dll)|(_d.dll)" 
    EXCLUDE 
    PATTERN ".git" EXCLUDE )
    
    # Release deployments do modify enviromental variables, other deployments do not.
    if(CPACK_PACKAGE_SUFFIX STREQUAL "") 
        # On install
        set (CPACK_NSIS_EXTRA_INSTALL_COMMANDS "Push \\\"MANTIDPATH\\\"
            Push \\\"A\\\"
            Push \\\"HKCU\\\"
            Push \\\"$INSTDIR\\\\bin\\\"
            Call EnvVarUpdate
            Pop  \\\$0
        
            Push \\\"PATH\\\"
            Push \\\"A\\\"
            Push \\\"HKCU\\\"
            Push \\\"$INSTDIR\\\\bin\\\"
            Call EnvVarUpdate
            Pop  \\\$0
            
            Push \\\"PATH\\\"
            Push \\\"A\\\"
            Push \\\"HKCU\\\"
            Push \\\"$INSTDIR\\\\${PVPLUGINS_DIR}\\\"
            Call EnvVarUpdate
            Pop  \\\$0
            
            Push \\\"PATH\\\"
            Push \\\"A\\\"
            Push \\\"HKCU\\\"
            Push \\\"$INSTDIR\\\\${PLUGINS_DIR}\\\"
            Call EnvVarUpdate
            Pop  \\\$0
            
            Push \\\"PV_PLUGIN_PATH\\\"
            Push \\\"A\\\"
            Push \\\"HKCU\\\"
            Push \\\"$INSTDIR\\\\${PVPLUGINS_DIR}\\\\${PVPLUGINS_DIR}\\\"
            Call EnvVarUpdate
            Pop  \\\$0
        
            CreateShortCut \\\"$DESKTOP\\\\MantidPlot.lnk\\\" \\\"$INSTDIR\\\\bin\\\\MantidPlot.exe\\\"
        
            CreateDirectory \\\"$INSTDIR\\\\logs\\\"
        
            CreateDirectory \\\"$INSTDIR\\\\docs\\\"
        ")
    # On unistall reverse stages listed above.
        set (CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS 
            "Push \\\"PATH\\\"
            Push \\\"R\\\"
            Push \\\"HKCU\\\"
            Push \\\"$INSTDIR\\\\bin\\\"
            Call un.EnvVarUpdate
            Pop  \\\$0
            
            Push \\\"PATH\\\"
            Push \\\"R\\\"
            Push \\\"HKCU\\\"
            Push \\\"$INSTDIR\\\\${PVPLUGINS_DIR}\\\"
            Call un.EnvVarUpdate
            Pop  \\\$0
            
            Push \\\"PATH\\\"
            Push \\\"R\\\"
            Push \\\"HKCU\\\"
            Push \\\"$INSTDIR\\\\${PLUGINS_DIR}\\\"
            Call un.EnvVarUpdate
            Pop  \\\$0
        
            Push \\\"MANTIDPATH\\\"
            Push \\\"R\\\"
            Push \\\"HKCU\\\"
            Push \\\"$INSTDIR\\\\bin\\\"
            Call un.EnvVarUpdate
            Pop  \\\$0

            Push \\\"PV_PLUGIN_PATH\\\"
            Push \\\"R\\\"
            Push \\\"HKCU\\\"
            Push \\\"$INSTDIR\\\\${PVPLUGINS_DIR}\\\\${PVPLUGINS_DIR}\\\"
            Call un.EnvVarUpdate
            Pop  \\\$0
        
            Delete \\\"$DESKTOP\\\\MantidPlot.lnk\\\"
        
            RMDir \\\"$INSTDIR\\\\logs\\\"
        
            RMDir \\\"$INSTDIR\\\\docs\\\"
        ")
    else ()
    set( CPACK_PACKAGE_INSTALL_DIRECTORY "MantidInstall${CPACK_PACKAGE_SUFFIX}")
    set( CPACK_NSIS_INSTALL_ROOT "C:")
    # On install
        set (CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
            CreateShortCut \\\"$DESKTOP\\\\MantidPlot.lnk\\\" \\\"$INSTDIR\\\\bin\\\\MantidPlot.exe\\\"
        
            CreateDirectory \\\"$INSTDIR\\\\logs\\\"
        
            CreateDirectory \\\"$INSTDIR\\\\docs\\\"
        ")
    # On unistall reverse stages listed above.
        set (CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
            Delete \\\"$DESKTOP\\\\MantidPlot.lnk\\\"
        
            RMDir \\\"$INSTDIR\\\\logs\\\"
        
            RMDir \\\"$INSTDIR\\\\docs\\\"
        ")
    endif()
