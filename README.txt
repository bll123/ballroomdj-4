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
  Building From Source

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

  This installation will not affect any BallroomDJ 3 installation.
  (Writing audio file tags is turned off upon conversion).

Known Issues:
  Windows
    - The marquee position is not saved when it is iconified (close the
      window instead).
  Linux
    - If an non-default audio sink is configured, and the default audio
      sink is changed, the audio will switch to the new default.  The
      audio will revert back to the configured audio sink when a new song
      starts playing.

Feedback:
  Leave your feedback, thoughts and ruminations at :
      https://ballroomdj.org/forum/viewforum.php?f=26

  You can also use the BallroomDJ 4 support function to send a message.

Copyright:
  Copyright 2021-2024 Brad Lanam Pleasant Hill, CA

Licenses:
    BDJ4        : zlib/libpng License
    libid3tag   : GPLv2 License
                  Changes made to the library available upon request.
    libmp4tag   : zlib/libpng License
    mongoose    : GPLv2 License
    qrcode      : MIT License (templates/qrcode/LICENSE)
    img/musicbrainz-logo.svg : GPLv2
    MacOS:
      ICU         : ICU
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

Building From Source:

  Download the source package from:
    https://sourceforge.net/projects/ballroomdj4/files/source/
  Unpack the package, and read the 'BUILD.txt' file.
  For Windows, the bdj4-src-win64-DATE.zip file is also needed.
  For MacOS, the bdj4-src-macos-DATE.zip file is also needed.
