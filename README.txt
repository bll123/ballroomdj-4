BDJ4 #VERSION# #BUILDDATE#

Contents
  Installation
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

Change Log:
  https://sourceforge.net/p/ballroomdj4/wiki/en-Change%20Log/

Converting BallroomDJ 3:
  https://sourceforge.net/p/ballroomdj4/wiki/en-Install-Converting%20BDJ3

Release Notes:

  4.3.3:
    MacOS Installation:
      The macos-pre-install-macports-v12.sh script must be run.
    Linux Installation:
      The linux-pre-install-v6.sh script must be run.

  4.3.2.4:
    The Configuration / General option 'BPM' has been set to MPM.
    If you want to display beats per minute, reset this configuration
    item back to BPM.

  Not yet implemented:
    - Auto-Organization.
    - Music Manager:
      - Audio Identification.

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
    mutagen     : GPLv2 License
    qrcode      : MIT License (templates/qrcode/LICENSE)
    Bento4      : GPLv2
    ICU         : ICU
    libid3tag   : GPLv2 License
                  Changes made to the library available upon request.
    libvorbis   : Xiph/BSD License
                  Changes made to the library available upon request.
    img/musicbrainz-logo.svg : GPLv2
    Windows:
      msys2     : BSD 3-Clause ( https://github.com/msys2/MSYS2-packages/blob/master/LICENSE )
      flac      : GPLv2, GPLv2.1, Xiph/BSD License
      ffmpeg    : GPLv3 License
      curl      : MIT License
      nghttp2   : MIT License
      libogg    : Xiph/BSD License
      opus      : Xiph/BSD License
      opusfile  : Xiph/BSD License
