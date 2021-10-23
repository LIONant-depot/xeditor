echo OFF
setlocal enabledelayedexpansion
set XEDITOR="%cd%"

rem --------------------------------------------------------------------------------------------------------
rem Set the color of the terminal to blue with yellow text
rem --------------------------------------------------------------------------------------------------------
COLOR 8E
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
powershell write-host -fore Cyan Welcome I am your XEDITOR dependency updater bot, let me get to work...
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
echo.

rem ------------------------------------------------------------
rem XGPU
rem ------------------------------------------------------------
:XGPU
rmdir "../dependencies/xGPU" /S /Q
git clone https://github.com/LIONant-depot/xGPU.git "../dependencies/xGPU"
if %ERRORLEVEL% GEQ 1 goto :ERROR

rem ------------------------------------------------------------
rem IconFontCppHeaders
rem ------------------------------------------------------------
:ICON_FONT_CPP_HEADERS
rmdir "../dependencies/IconFontCppHeaders" /S /Q
git clone https://github.com/juliettef/IconFontCppHeaders.git "../dependencies/IconFontCppHeaders"
if %ERRORLEVEL% GEQ 1 goto :ERROR

rem ------------------------------------------------------------
rem XCORE
rem ------------------------------------------------------------
rmdir "../dependencies/xcore" /S /Q
git clone https://gitlab.com/LIONant/xcore.git "../dependencies/xCore"
if %ERRORLEVEL% GEQ 1 goto :PAUSE
cd ../dependencies/xcore/builds
call UpdateDependencies.bat "return"
if %ERRORLEVEL% GEQ 1 goto :PAUSE
cd /d %XEDITOR%

rem ------------------------------------------------------------
rem DONWLOAD IMGUI TOOL TO BUILD FONTS
rem ------------------------------------------------------------
cd /d %XEDITOR%
set FONTS= "%XEDITOR%/../dependencies/xcore/dependencies/tracy/imgui/misc"
cd /d %FONTS%
if %ERRORLEVEL% GEQ 1 goto :ERROR
bitsadmin /transfer binary_to_compressed_c /download /priority high https://raw.githubusercontent.com/ocornut/imgui/master/misc/fonts/binary_to_compressed_c.cpp "%cd%\binary_to_compressed_c.cpp"
if %ERRORLEVEL% GEQ 1 goto :ERROR

rem ------------------------------------------------------------
rem DONWLOAD ACTUAL FONTS
rem ------------------------------------------------------------
cd /d %FONTS%
bitsadmin /transfer fa-regular-400 /download /priority high https://raw.githubusercontent.com/FortAwesome/Font-Awesome/master/webfonts/fa-regular-400.ttf "%cd%\fa-regular-400.ttf"
if %ERRORLEVEL% GEQ 1 goto :ERROR

bitsadmin /transfer fa-solid-900 /download /priority high https://raw.githubusercontent.com/FortAwesome/Font-Awesome/master/webfonts/fa-solid-900.ttf "%cd%\fa-solid-900.ttf"
if %ERRORLEVEL% GEQ 1 goto :ERROR

bitsadmin /transfer kenney-icon-font /download /priority high https://raw.githubusercontent.com/nicodinh/kenney-icon-font/master/fonts/kenney-icon-font.ttf "%cd%\kenney-icon-font.ttf"
if %ERRORLEVEL% GEQ 1 goto :ERROR

bitsadmin /transfer MaterialIcons-Regular /download /priority high https://raw.githubusercontent.com/google/material-design-icons/master/font/MaterialIcons-Regular.ttf "%cd%\MaterialIcons-Regular.ttf"
if %ERRORLEVEL% GEQ 1 goto :ERROR

rem ------------------------------------------------------------
rem FIND VISUAL STUDIO
rem ------------------------------------------------------------
:FIND_VS
set FONTS= "%XEDITOR%/../dependencies/xcore/dependencies/tracy/imgui/misc"
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
powershell write-host -fore White XEDITOR - FINDING VISUAL STUDIO / MSBuild
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
cd /d %XEDITOR%
for /f "usebackq tokens=*" %%i in (`..\dependencies\xcore\bin\vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set InstallDir=%%i
)

if exist "%InstallDir%\VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt" (
  set /p Version=<"%InstallDir%\VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt"

  rem Trim
  set Version=!Version: =!
)

CALL "%InstallDir%\VC\Auxiliary\Build\vcvarsall.bat" x64
if %ERRORLEVEL% GEQ 1 goto :ERROR

IF EXIST "%InstallDir%" ( 
    echo OK 
    GOTO :COMPILATION
    )
echo Failed to find MSBuild!!! 
GOTO :ERROR

:COMPILATION
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
powershell write-host -fore White XEDITOR - COMPILING DEPENDENCIES
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
echo.
cd /d %FONTS%
cl.exe binary_to_compressed_c.cpp
if %ERRORLEVEL% GEQ 1 goto :ERROR

binary_to_compressed_c.exe MaterialIcons-Regular.ttf MaterialIcons_Regular > MaterialIcons_Regular.h 
if %ERRORLEVEL% GEQ 1 goto :ERROR

binary_to_compressed_c.exe kenney-icon-font.ttf kenney_icon_font > kenney_icon_font.h 
if %ERRORLEVEL% GEQ 1 goto :ERROR

binary_to_compressed_c.exe fa-regular-400.ttf fa_regular_400 > fa_regular_400.h 
if %ERRORLEVEL% GEQ 1 goto :ERROR

binary_to_compressed_c.exe fa-solid-900.ttf fa_solid_900 > fa_solid_900.h 
if %ERRORLEVEL% GEQ 1 goto :ERROR

:DONE
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
powershell write-host -fore White XEDITOR - DONE!!
powershell write-host -fore White ------------------------------------------------------------------------------------------------------
goto :PAUSE

:ERROR
powershell write-host -fore Red ------------------------------------------------------------------------------------------------------
powershell write-host -fore Red XEDITOR - ERROR!!
powershell write-host -fore Red ------------------------------------------------------------------------------------------------------

:PAUSE
rem if no one give us any parameters then we will pause it at the end, else we are assuming that another batch file called us
if %1.==. pause

