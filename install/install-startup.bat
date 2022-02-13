@echo off

set tdir="%TEMP%"

echo -- BDJ4 Installation Startup

set guidisabled=""
set reinstall=""

:procargs
if "%1" == "" ( goto endargs )
if "%1" == "--reinstall" (
  set reinstall="--reinstall"
)
if "%1" == "--cli" (
  set guidisabled="--guidisabled"
)
shift
goto procargs
:endargs

cd %tdir%

cd bdj4-install

echo -- Starting installer.
.\bin\bdj4.exe --installer --unpackdir %tdir%\bdj4-install %reinstall% %guidisabled%

echo -- Cleaning temporary files.
if exist "%tdir%\bdj4-install.zip" (
  del /q "%tdir%\bdj4-install.zip"
)
if exist "%tdir%\miniunz.exe" (
  del /q "%tdir%\miniunz.exe"
)
if exist "%tdir%\bdj4-unzip.log" (
  del /q "%tdir%\bdj4-unzip.log"
)

exit 0
