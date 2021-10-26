@echo off
pushd "%~dp0"

rmdir /s /q artifacts

rmdir /s /q vs2019\dll\.vs
del vs2019\dll\*.vcxproj.user

rmdir /s /q vs2019\exe\.vs
rmdir /s /q vs2019\exe\Win32
rmdir /s /q vs2019\exe\x64
del vs2019\exe\*.vcxproj.user

popd
