@echo off

set tmpdir="%TEMP%"

echo -- BallroomDJ 4 Installation Startup

cd %tmpdir%

cd bdj4-install

echo -- Starting graphical installer.
.\bin\bdj4.exe --installer

echo -- Cleaning temporary files.
if exist "%tmpdir%\bdj4-install.zip" (
  del /q "%tmpdir%\bdj4-install.zip"
)
if exist "%tmpdir%\miniunz.exe" (
  del /q "%tmpdir%\miniunz.exe"
)
if exist "%tmpdir%\bdj4-unzip.log" (
  del /q "%tmpdir%\bdj4-unzip.log"
)

exit 0
