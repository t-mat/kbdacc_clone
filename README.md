# kbdacc_clone

Clone of classic [Tetsuya Kamei](http://www.jsdlab.co.jp/~kamei/)'s `kbdacc` which accelerates key repeat time.


## Building

Prerequisites

- Windows 10 (x64)
- Visual C++ 2019
- `git`

```
cd /d "%USERPROFILE%\Documents"
git clone https://github.com/t-mat/kbdacc_clone.git
cd kbdacc_clone
.\build.bat
start "" ".\artifacts\Release\kbdacc.exe"
```


## Usage

- When it's running, you can see `kbdacc` icon in the system tray.
- To quit the app, click the icon and select "Quit".

There're two environment variables for configuration.  If you need to use it, set these environment variables before invoking `kbdacc.exe`.

- `KBDACC_DELAY` : Delay time of first key down to key repeat in milliseconds.  Default value is `200`.
- `KBDACC_REPEAT` : Key repeat time in milliseconds.  Default value is `10`.
