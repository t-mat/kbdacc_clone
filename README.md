# kbdacc_clone

Clone of classic [Tetsuya Kamei](http://www.jsdlab.co.jp/~kamei/)'s `kbdacc` which accelerates key repeat rate.


## Building

Prerequisites

- Windows 10 (x64) [version 1809](https://en.wikipedia.org/wiki/Windows_10_version_1809) or greater.
- Visual C++ 2019


### Command Prompt

```
cd /d "%USERPROFILE%\Documents"
curl -JLO https://github.com/t-mat/kbdacc_clone/archive/refs/heads/main.zip
tar -xf kbdacc_clone-main.zip
cd kbdacc_clone-main
.\build.bat
start "" ".\artifacts\Release\kbdacc.exe"
```


### Visual Studio

```
cd /d "%USERPROFILE%\Documents"
curl -JLO https://github.com/t-mat/kbdacc_clone/archive/refs/heads/main.zip
tar -xf kbdacc_clone-main.zip
cd kbdacc_clone-main
start "" ".\vs2019\exe\kbdacc.sln"
```

- From main menu, select "Build > Batch Build".
- In the batch build dialog, "Select All" and "Build".


## Usage

- When it's running, you can see `kbdacc` icon in the system tray.
- To quit the app, click the icon and select "Quit".

There're two environment variables for configuration.  To use them, set these environment variables before invoking `kbdacc.exe`.

- `KBDACC_DELAY` : Repeat delay in milliseconds.  Default value is `200`.
- `KBDACC_REPEAT` : Repeat rate in milliseconds.  Default value is `10`.
