BDJ4 4.0.0

BallroomDJ 4 is currently under development.

Alpha Release: 20220627

  Alpha releases are mostly untested works in progress.

  Alpha releases may not reflect what the final product will look like,
  and may not reflect what features will be present in the final product.

  Alpha releases are not backwards compatible with each other.
  Do a re-install rather than an upgrade.

  Anything can and will change.

Installation:
  Windows:
    a) Run the BallroomDJ 4 installer.

  Linux:
    a) Download the linux-pre-install.sh script from:
        https://sourceforge.net/projects/ballroomdj4/files/linux-pre-install.sh/download
      and run it.
      This script uses sudo to install the required packages.
    b) Run the BallroomDJ 4 installer.

  MacOS:
    The BallroomDJ 4 installer will not work on MacOS without step (a).

    a) Download the macos-pre-install.sh script from:
        https://sourceforge.net/projects/ballroomdj4/files/macos-pre-install.sh/download
      and run it.
      This script uses sudo to install the required packages (from MacPorts).
      [ I have not tested with 'brew'.  That's in the future someday. ]
    b) Run the BallroomDJ 4 installer.

Release Notes: 20220623
  This release represents a working player user interface,
  the configuration user interface,
  and a partly functional management interface:
    - song list editor (not all features present)
    - sequence editor
    - playlist management
    - music manager / song editor

  This installation will not affect any BallroomDJ 3 installation.
  (The features that rename audio files and write audio file tags are
   not implemented yet).

Known Issues:
  MacOS
    - The windows will not de-iconify (use right-click, show all windows).

Feedback:
  Leave your feedback, thoughts and ruminations at :
      https://ballroomdj.org/forum/viewforum.php?f=26

  You can also use the BallroomDJ 4 support function to send a message.

LICENSES
    BDJ4    : zlib/libpng License
    mutagen : GPLv2 License
    qrcode  : MIT License (templates/qrcode/LICENSE)
    Windows:
      msys2      : BSD 3-Clause ( https://github.com/msys2/MSYS2-packages/blob/master/LICENSE )
      libmad     : GPL License
      libmp3lame : LGPL https://lame.sourceforge.net/
      ffmpeg     : GPLv3 License
      curl       : MIT License
      c-ares     : MIT License
      nghttp2    : MIT License
      zlib       : zlib/libpng License
      zstd       : BSD License
