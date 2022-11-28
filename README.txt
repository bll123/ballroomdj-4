BDJ4 4.0.0
Alpha Release: 2022-11---

Contents
  Installation
  Release Information
  Release Notes
  Converting BallroomDJ 3
  Known Issues
  Feedback
  Licenses

Installation:
  Windows:
    https://sourceforge.net/p/ballroomdj4/wiki/en-Install-Windows/
  MacOS:
    https://sourceforge.net/p/ballroomdj4/wiki/en-Install-MacOS/
  Linux:
    https://sourceforge.net/p/ballroomdj4/wiki/en-Install-Linux/

Release Information

  BallroomDJ 4 is currently under development.

  Alpha releases are mostly untested works in progress.

  Alpha releases may not reflect what the final product will look like,
  and may not reflect what features will be present in the final product.

  Alpha releases are not backwards compatible with each other.
  Do a re-install rather than an upgrade.

  Anything can and will change.

Release Notes:
  See the file ChangeLog.txt for a full list of changes.

  Not yet implemented:
    - Auto-Organization.
    - iTunes:
      - Database Update: Update from iTunes data.
      - Song List Editor: Import from iTunes.
    - Song List Editor:
      - Batch editing.
      - Export for BDJ/Import from BDJ.
    - Music Manager:
      - Apply Adjustments (speed, song start, song end) to a song.
      - Audio Identification.
    - Export a playlist as MP3 files.

  This installation will not affect any BallroomDJ 3 installation.
  (The features that rename audio files is not implemented yet).
  (Writing audio file tags is turned off upon conversion, and the
   'Write BDJ3 Compatible Audio Tags' is on).

Converting BallroomDJ 3:

  If you have a recent version of BallroomDJ 3, and the installer is
  able to locate your installation, the BallroomDJ 3 data files will
  automatically be converted during the installation process.
  The BallroomDJ 3 installation is not changed and may still be used.

  If you have an older version of BallroomDJ 3, and the installer says
  that your "BDJ3 database version is too old", use the following
  process.  This process is set up to preserve your original BallroomDJ
  installation and make sure it is not changed.
    - Copy your entire BallroomDJ 3 folder to another location and/or name.
      (e.g. BallroomDJ to BDJ3Temp)
    - Rename the BallroomDJ shortcut on the desktop to a new name
      (e.g. "BallroomDJ original").
    - Download the latest version of BallroomDJ 3.
    - Run the installer, but choose the new location as the installation
      location.  The BallroomDJ 3 installation process will upgrade your
      database and data files to the latest version.
    - Delete the BallroomDJ shortcut on the desktop (it is pointing to
      the new location).
    - Rename the original BallroomDJ shortcut back the way it was
      (e.g. "BallroomDJ original" to BallroomDJ).
    - Now run the BDJ4 installation process again.  For the BallroomDJ 3
      location, choose the new location for BallroomDJ 3.
    - Remove the new location of BallroomDJ 3 (e.g. BDJ3Temp).  Your
      original BallroomDJ installation is untouched and can still be used.

Known Issues:
  Windows
    - The marquee position is not saved when it is iconified (close the
      window instead).

Feedback:
  Leave your feedback, thoughts and ruminations at :
      https://ballroomdj.org/forum/viewforum.php?f=26

  You can also use the BallroomDJ 4 support function to send a message.

LICENSES
    BDJ4    : zlib/libpng License
    mutagen : GPLv2 License
    qrcode  : MIT License (templates/qrcode/LICENSE)
    img/musicbrainz-logo.svg : GPLv2
    Windows:
      msys2      : BSD 3-Clause ( https://github.com/msys2/MSYS2-packages/blob/master/LICENSE )
      ffmpeg     : GPLv3 License
      curl       : MIT License
      c-ares     : MIT License
      nghttp2    : MIT License
      zlib       : zlib/libpng License
      zstd       : BSD License
