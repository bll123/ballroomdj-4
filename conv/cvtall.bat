@ECHO OFF
REM windows conversion script

set dir=%1

if "%dir%" == "" (
  echo "No ballroomdj directory"
  exit /B 1
)

set BITS=32
if exist "C:\Program Files (x86)" (
  set bits=64
)

set tclsh=%dir%\..\windows\%bits%\tcl\bin\tclsh.exe

if ! exists %tclsh% (
  echo "Unable to locate tclsh.exe"
  exit /B 1
)

if ! exist data (
  echo "No bdj4 data directory"
  exit /B 1
)

if ! exist conv (
  echo "No bdj4 conv directory"
  exit /B 1
)

"%tclsh%" ./conv/configconv.tcl $dir
"%tclsh%" ./conv/danceconv.tcl $dir
"%tclsh%" ./conv/dbconv.tcl $dir
"%tclsh%" ./conv/genreconv.tcl $dir
"%tclsh%" ./conv/levelsconv.tcl $dir
"%tclsh%" ./conv/mlistconv.tcl $dir
"%tclsh%" ./conv/playlistconv.tcl $dir
"%tclsh%" ./conv/ratingconv.tcl $dir
"%tclsh%" ./conv/seqconv.tcl $dir
"%tclsh%" ./conv/sortoptconv.tcl $dir
"%tclsh%" ./conv/autoselconv.tcl $dir
"%tclsh%" ./conv/typeconv.tcl $dir
"%tclsh%" ./conv/statusconv.tcl $dir
