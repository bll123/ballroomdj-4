@ECHO OFF
REM windows conversion script

set bdj3dir=%1
set datatopdir=%2

if not exist %bdj3dir% (
  echo ERROR: No BallroomDJ directory.
  exit /B 1
)

if not exist %bdj3dir%/data (
  echo ERROR: No BallroomDJ 3 data directory.
  exit /B 1
)

if not exist %datatopdir% (
  echo ERROR: No target directory.
  exit /B 1
)

set bdj3dir=%bdj3dir%/data

set bits=32
if exist "C:\Program Files (x86)" (
  set bits=64
)

set tclsh="%bdj3dir%\..\windows\%bits%\tcl\bin\tclsh.exe"
if not exist %tclsh% (
  set tclsh="%bdj3dir%\..\..\windows\%bits%\tcl\bin\tclsh.exe"
  if not exist %tclsh% (
    echo ERROR: Unable to locate tclsh.exe
    exit /B 1
  )
)

echo Located tclsh: %tclsh%

cd %datatopdir%

if not exist "data" (
  echo ERROR: No BDJ4 data directory
  exit /B 1
)

if not exist "conv" (
  echo ERROR: No BDJ4 conv directory
  exit /B 1
)

%tclsh% ./conv/configconv.tcl %bdj3dir%
%tclsh% ./conv/danceconv.tcl %bdj3dir%
%tclsh% ./conv/dbconv.tcl %bdj3dir%
%tclsh% ./conv/genreconv.tcl %bdj3dir%
%tclsh% ./conv/levelsconv.tcl %bdj3dir%
%tclsh% ./conv/mlistconv.tcl %bdj3dir%
%tclsh% ./conv/playlistconv.tcl %bdj3dir%
%tclsh% ./conv/ratingconv.tcl %bdj3dir%
%tclsh% ./conv/seqconv.tcl %bdj3dir%
%tclsh% ./conv/sortoptconv.tcl %bdj3dir%
%tclsh% ./conv/autoselconv.tcl %bdj3dir%
%tclsh% ./conv/typeconv.tcl %bdj3dir%
%tclsh% ./conv/statusconv.tcl %bdj3dir%
echo Conversion Complete
echo OK

exit 0
