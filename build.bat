@setlocal enabledelayedexpansion
@echo off
pushd "%~dp0"
set /a errorno=1


rem
rem Set Visual C++ environment for Windows, desktop, x64.
rem
rem https://github.com/Microsoft/vswhere
rem https://github.com/microsoft/vswhere/wiki/Find-VC#batch
rem

set "vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%vswhere%" (
  echo vswhere.exe doesn't exist.  Please install Visual Studio.
  set /a errorno=1
  goto :ERROR
)

rem Set InstallDir
rem DO NOT CHANGE the variable name "InstallDir".
rem This variable is used in vcvars64.bat and its friends.
for /f "usebackq tokens=*" %%i in (`"%vswhere%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set "InstallDir=%%i"
)
if "%InstallDir%" == "" (
  echo Can't find Visual C++.   Please install the latest version of Visual C++.
  goto :ERROR
)


echo call "%InstallDir%\VC\Auxiliary\Build\vcvars64.bat"
     call "%InstallDir%\VC\Auxiliary\Build\vcvars64.bat" || goto :ERROR

rem
rem Build *.sln file by msbuild
rem
rem https://docs.microsoft.com/visualstudio/msbuild/msbuild-command-line-reference
rem


if not exist "%~dp0artifacts\Release" (
       mkdir "%~dp0artifacts\Release"
)
if not exist "%~dp0artifacts\Debug" (
       mkdir "%~dp0artifacts\Debug"
)
copy media\*.ico "%~dp0artifacts\Release\"
copy media\*.ico "%~dp0artifacts\Debug\"

pushd vs2019\exe
set "SlnFile=kbdacc.sln"

echo msbuild "%SlnFile%" /p:Platform=x64   /p:Configuration=Release
     msbuild "%SlnFile%" /p:Platform=x64   /p:Configuration=Release /nologo /v:quiet /m /t:Clean,Build || goto :ERROR
echo msbuild "%SlnFile%" /p:Platform=Win32 /p:Configuration=Release
     msbuild "%SlnFile%" /p:Platform=Win32 /p:Configuration=Release /nologo /v:quiet /m /t:Clean,Build || goto :ERROR
echo msbuild "%SlnFile%" /p:Platform=x64   /p:Configuration=Debug
     msbuild "%SlnFile%" /p:Platform=x64   /p:Configuration=Debug   /nologo /v:quiet /m /t:Clean,Build || goto :ERROR
echo msbuild "%SlnFile%" /p:Platform=Win32 /p:Configuration=Debug
     msbuild "%SlnFile%" /p:Platform=Win32 /p:Configuration=Debug   /nologo /v:quiet /m /t:Clean,Build || goto :ERROR

popd

echo.
echo "%~dp0artifacts\Release" contains the following artifacts.
echo.
echo dir /b "%~dp0artifacts\Release"
     dir /b "%~dp0artifacts\Release"
echo.

echo.
echo "%~dp0artifacts\Debug" contains the following artifacts.
echo.
echo dir /b "%~dp0artifacts\Debug"
     dir /b "%~dp0artifacts\Debug"
echo.

echo Build Status - SUCCEEDED
set /a errorno=0
goto :END


:ERROR
echo Abort by error.
echo Build Status - ERROR


:END
popd
exit /B %errorno%
