setlocal enableextensions enabledelayedexpansion
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: WINDOWS SCRIPT TO DRIVE THE JENKINS BUILDS OF MANTID.
::
:: Notes:
::
:: WORKSPACE & JOB_NAME are environment variables that are set by Jenkins.
:: BUILD_THREADS & PARAVIEW_DIR should be set in the configuration of each slave.
:: CMake, git & git-lfs should be on the PATH
::
:: All nodes currently have PARAVIEW_DIR=5.2.0 and PARAVIEW_NEXT_DIR=5.3.0-RC1
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
call cmake.exe --version
set VS_VERSION=14

:: While we transition between VS 2012 & 2015 we need to be able to clean the build directory
:: if the previous build was not with the same compiler. Find grep for later
for /f "delims=" %%I in ('where git') do @set GIT_EXE_DIR=%%~dpI
set GIT_ROOT_DIR=%GIT_EXE_DIR:~0,-4%
set GREP_EXE=%GIT_ROOT_DIR%bin\grep.exe
echo %sha1%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Environment setup
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Source the VS setup script
set VS_VERSION=14
call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64
set CM_GENERATOR=Visual Studio 14 2015 Win64
set PARAVIEW_DIR=%PARAVIEW_NEXT_DIR%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Set up the location for local object store outside of the build and source
:: tree, which can be shared by multiple builds.
:: It defaults to a MantidExternalData directory within the USERPROFILE
:: directory. It can be overridden by setting the MANTID_DATA_STORE environment
:: variable.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if NOT DEFINED MANTID_DATA_STORE (
  set MANTID_DATA_STORE=%USERPROFILE%\MantidExternalData
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Check job requirements from the name
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set CLEANBUILD=
set BUILDPKG=
if not "%JOB_NAME%" == "%JOB_NAME:clean=%" (
  set CLEANBUILD=yes
  set BUILDPKG=yes
)

:: BUILD_PACKAGE can be provided as a job parameter on the pull requests
if not "%JOB_NAME%" == "%JOB_NAME:pull_requests=%" (
  if not "%BUILD_PACKAGE%" == "%BUILD_PACKAGE:true=%" (
    set BUILDPKG=yes
  ) else (
    set BUILDPKG=no
  )
)
:: Never want package for debug builds
if not "%JOB_NAME%" == "%JOB_NAME:debug=%" (
  set BUILDPKG=no
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Setup the build directory
:: For a clean build the entire thing is removed to guarantee it is clean. All
:: other build types are assumed to be incremental and the following items
:: are removed to ensure stale build objects don't interfere with each other:
::   - build/bin: if libraries are removed from cmake they are not deleted
::                   from bin and can cause random failures
::   - build/ExternalData/**: data files will change over time and removing
::                            the links helps keep it fresh
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set BUILD_DIR=%WORKSPACE%\build

if EXIST %BUILD_DIR%\CMakeCache.txt (
  call "%GREP_EXE%" CMAKE_LINKER:FILEPATH %BUILD_DIR%\CMakeCache.txt > compiler_version.log
  call "%GREP_EXE%" %VS_VERSION% compiler_version.log
  if ERRORLEVEL 1 (
    set CLEANBUILD=yes
    echo Previous build used a different compiler. Performing a clean build
  ) else (
    echo Previous build used the same compiler. No need to clean
  )
)

if "!CLEANBUILD!" == "yes" (
  rmdir /S /Q %BUILD_DIR%
)

if EXIST %BUILD_DIR% (
  rmdir /S /Q %BUILD_DIR%\bin %BUILD_DIR%\ExternalData
) else (
  md %BUILD_DIR%
)

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Packaging options
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set PACKAGE_DOCS=
if "%BUILDPKG%" == "yes" (
  set PACKAGE_DOCS=-DPACKAGE_DOCS=ON
)

cd %BUILD_DIR%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Clean up any artifacts from last build so that if it fails
:: they don't get archived again.
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
del /Q *.exe

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Check the required build configuration
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
set BUILD_CONFIG=
if not "%JOB_NAME%"=="%JOB_NAME:debug=%" (
  set BUILD_CONFIG=Debug
) else (
if not "%JOB_NAME%"=="%JOB_NAME:relwithdbg=%" (
  set BUILD_CONFIG=RelWithDbg
) else (
    set BUILD_CONFIG=Release
    ))

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: CMake configuration
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Note the exception: Vates disabled in Debug mode for now.
if not "%JOB_NAME%"=="%JOB_NAME:debug=%" (
  set VATES_OPT_VAL=OFF
) else (
  set VATES_OPT_VAL=ON
)
call cmake.exe -G "%CM_GENERATOR%" -DCONSOLE=OFF -DENABLE_CPACK=ON -DMAKE_VATES=%VATES_OPT_VAL% -DParaView_DIR=%PARAVIEW_DIR% -DMANTID_DATA_STORE=!MANTID_DATA_STORE! -DUSE_PRECOMPILED_HEADERS=ON -DENABLE_FILE_LOGGING=OFF %PACKAGE_DOCS% ..
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build step
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
call %BUILD_DIR%\buildenv.bat
msbuild /nologo /m:%BUILD_THREADS% /nr:false /p:Configuration=%BUILD_CONFIG% Mantid.sln
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Run the tests
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Remove the user properties file just in case anything polluted it
set USERPROPS=bin\%BUILD_CONFIG%\Mantid.user.properties
del %USERPROPS%

call ctest.exe -C %BUILD_CONFIG% -j%BUILD_THREADS% --schedule-random --output-on-failure
if ERRORLEVEL 1 exit /B %ERRORLEVEL%

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Run docs-tests if in the special Debug builds
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
echo Note: not running doc-test target as it currently takes too long
:: if not "%JOB_NAME%"=="%JOB_NAME:debug=%" (
::   call cmake.exe --build . --target StandardTestData
::   call cmake.exe --build . --target docs-test
:: )

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Create the install kit if required
:: Disabled while it takes 10 minutes to create & 5-10 mins to archive!
:: Just create the docs to check they work
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

if "%BUILDPKG%" == "yes" (
  :: Build offline documentation
  msbuild /nologo /nr:false /p:Configuration=%BUILD_CONFIG% docs/docs-qthelp.vcxproj
  :: Ignore errors as the exit code of msbuild is wrong here.
  :: It always marks the build as a failure even thought the MantidPlot exit
  :: code is correct!
  echo Building package
  cpack.exe -C %BUILD_CONFIG% --config CPackConfig.cmake
)
