BDJ4 #VERSION# #BUILDDATE#

Contents
  Installation
  Getting Started
  Change Log
  Converting BallroomDJ 3
  Release Notes
  Known Issues
  Feedback
  Copyright
  Licenses

Installation:
  Windows:
    https://sourceforge.net/p/ballroomdj4/wiki/en-Install-Windows/
  Mac OS:
    https://sourceforge.net/p/ballroomdj4/wiki/en-Install-MacOS/
  Linux:
    https://sourceforge.net/p/ballroomdj4/wiki/en-Install-Linux/

Getting Started:
    https://sourceforge.net/p/ballroomdj4/wiki/en-Install-Getting%20Started/

Change Log:
  https://sourceforge.net/p/ballroomdj4/wiki/en-Change%20Log/

Converting BallroomDJ 3:
  https://sourceforge.net/p/ballroomdj4/wiki/en-Install-Converting%20BDJ3

Release Notes:

  4.4.8:
    There have been a lot of internal changes.
    Chances that something is broken is higher than usual.

  4.4.5:
    Windows account names with international characters are now working!

  4.4.4:
    Please upgrade to version 4.4.4 (or later) if you are using version 4.4.3.

  4.4.3:
    The mutagen audio tag interface has been removed.
    Windows: To clean up the python installations (no longer needed),
      open a cmd.exe window, and run:
        %USERPROFILE%\BDJ4\install\win-clean-python.bat
      This is a manual process, as python may be installed and
      needed by other applications.

  4.4.2 and later:
    Please upgrade to version 4.4.2 (or later) if you are using
      version 4.3.3.2 or later.
    Linux Installation:
      Run the linux-pre-install-v12.sh script.
    MacOS Installation:
      Run the macos-pre-install-macports-v17.sh script.

  4.3.2.4:
    The Configuration / General option 'BPM' has been set to MPM.
    If you want to display beats per minute, reset this configuration
    item back to BPM.

  Not yet implemented:
    - Auto-Organization.

  This installation will not affect any BallroomDJ 3 installation.
  (The features that rename audio files is not implemented yet).
  (Writing audio file tags is turned off upon conversion, and the
   'Write BDJ3 Compatible Audio Tags' is on).

Known Issues:
  Windows
    - The marquee position is not saved when it is iconified (close the
      window instead).

Feedback:
  Leave your feedback, thoughts and ruminations at :
      https://ballroomdj.org/forum/viewforum.php?f=26

  You can also use the BallroomDJ 4 support function to send a message.

Copyright:
  Copyright 2021-2023 Brad Lanam Pleasant Hill, CA

Licenses:
    BDJ4        : zlib/libpng License
    ICU         : ICU
    libid3tag   : GPLv2 License
                  Changes made to the library available upon request.
    libmp4tag   : zlib/libpng License
    mongoose    : GPLv2 License
    qrcode      : MIT License (templates/qrcode/LICENSE)
    img/musicbrainz-logo.svg : GPLv2
    Windows:
      curl      : MIT License
      ffmpeg    : GPLv3 License
      flac      : GPLv2, GPLv2.1, Xiph/BSD License
      libogg    : Xiph/BSD License
      libvorbis : Xiph/BSD License
                  Changes made to the library available upon request.
      msys2     : BSD 3-Clause ( https://github.com/msys2/MSYS2-packages/blob/master/LICENSE )
      nghttp2   : MIT License
      opusfile  : Xiph/BSD License
      opus      : Xiph/BSD License
