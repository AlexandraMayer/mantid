@echo off
if "%1" == "" goto usage

SETLOCAL
if /i %1 == x86 SET MSI_NAME="Mantid.msi" && SET WIX_LIB="C:\Program Files\Windows Installer XML\bin\wixui.wixlib"
if /i %1 == amd64 SET MSI_NAME="Mantid64.msi" && SET WIX_LIB="C:\Program Files (x86)\Windows Installer XML\bin\wixui.wixlib"

if /i not %1 == x86 (
    if /i not %1 == amd64 (
        goto usage
    )
)

svn update > build_number.txt
python generateWxs.py
if errorlevel 1 goto wxs_error
candle tmp.wxs  
if errorlevel 1 goto candle_error
light -out %MSI_NAME% tmp.wixobj %WIX_LIB% -loc WixUI_en-us.wxl
if errorlevel 1 goto light_error
python msiSize.py %MSI_NAME%
ENDLOCAL
goto :end

:wxs_error
echo "Error creating wxs file"
goto failed
:candle_error
echo "Error processing wxs file"
goto failed
:light_error
echo "MSI creation failed"
goto failed
:usage
echo "Usage: build.bat x86|amd64"
goto failed

:failed
ENDLOCAL
exit(1)

:end
ENDLOCAL
echo "MSI build succeeded"
exit(0)