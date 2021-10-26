@setlocal enabledelayedexpansion
pushd "%~dp0"

for /f "tokens=*" %%a in ('git describe --tags') do (
    set TAG=%%a
)

mkdir staging
pushd staging

copy ..\README.md .
copy ..\LICENSE .
copy ..\artifacts\Release\*.* .

7z a ..\kbdacc_clone_%TAG%.zip *.*

popd
rmdir /S /Q staging

popd
