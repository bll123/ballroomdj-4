@echo off

set tdir="%TEMP%"

echo -- BDJ4 Installation Startup

set guidisabled=""
set reinstall=""
set logstderr=""

cd %tdir%

cd bdj4-install

echo -- Starting installer.
.\bin\bdj4.exe --bdj4installer --unpackdir %tdir%\bdj4-install %*

echo -- Cleaning temporary files.
if exist "%tdir%\bdj4-install.cab" (
  del /q "%tdir%\bdj4-install.cab"
)
if exist "%tdir%\bdj4-expand.log" (
  del /q "%tdir%\bdj4-expand.log"
)

exit 0
