@echo off
pushd "%~dp0"

del artifacts\Debug\*.exe
del artifacts\Debug\*.exe32
del artifacts\Debug\*.dll
del artifacts\Debug\*.dll32

del artifacts\Release\*.exe
del artifacts\Release\*.exe32
del artifacts\Release\*.dll
del artifacts\Release\*.dll32

rmdir /s /q vs2019\dll\.vs
del vs2019\dll\*.vcxproj.user

rmdir /s /q vs2019\exe\.vs
rmdir /s /q vs2019\exe\Win32
rmdir /s /q vs2019\exe\x64
del vs2019\exe\*.vcxproj.user

popd
