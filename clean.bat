@echo off
pushd "%~dp0"

rmdir /s /q artifacts                 2>nul

rmdir /s /q vs2019\dll\.vs            2>nul
del         vs2019\dll\*.vcxproj.user 2>nul
rmdir /s /q vs2019\exe\.vs            2>nul
rmdir /s /q vs2019\exe\Win32          2>nul
rmdir /s /q vs2019\exe\x64            2>nul
del         vs2019\exe\*.vcxproj.user 2>nul

rmdir /s /q vs2022\dll\.vs            2>nul
del         vs2022\dll\*.vcxproj.user 2>nul
rmdir /s /q vs2022\exe\.vs            2>nul
rmdir /s /q vs2022\exe\Win32          2>nul
rmdir /s /q vs2022\exe\x64            2>nul
del         vs2022\exe\*.vcxproj.user 2>nul

popd
