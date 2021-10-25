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
rem Build each *.sln file by msbuild for x64 and Win32 with release configuration.
rem
rem https://docs.microsoft.com/visualstudio/msbuild/msbuild-command-line-reference
rem

cd vs2019\exe
set "SlnFile=kbdacc.sln"

echo msbuild "%SlnFile%" /nologo /v:quiet /m /p:Configuration=Release /p:Platform=x64 /t:Clean,Build || goto :ERROR
     msbuild "%SlnFile%" /nologo /v:quiet /m /p:Configuration=Release /p:Platform=x64 /t:Clean,Build || goto :ERROR
echo msbuild "%SlnFile%" /nologo /v:quiet /m /p:Configuration=Release /p:Platform=Win32 /t:Clean,Build || goto :ERROR
     msbuild "%SlnFile%" /nologo /v:quiet /m /p:Configuration=Release /p:Platform=Win32 /t:Clean,Build || goto :ERROR

echo.
echo "%~dp0artifacts\Release" contains the following artifacts.
echo.
echo dir /b "%~dp0artifacts\Release"
     dir /b "%~dp0artifacts\Release"
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
