#!/usr/bin/tclsh

proc mkrandcolor { } {
  set r [expr int (rand()*256.0)];
  set g [expr int (rand()*256.0)];
  set b [expr int (rand()*256.0)];
  set col [format "#%02x%02x%02x" $r $g $b]
  return $col;
}

proc copyimages { todir col } {
  set fnlist [glob -directory templates/img *.svg]
  foreach fn $fnlist {
    set nfn [file join $todir [file tail $fn]]
    file copy -force $fn $nfn
  }
}

if { $argc != 2 } {
  puts "usage: $argv0 <bdj3dir> <datatopdir>"
  exit 1
}

set topdir [pwd]
set bdj3dir [lindex $argv 0]
if { ! [file exists $bdj3dir] || ! [file isdirectory $bdj3dir] } {
  puts "Invalid directory $bdj3dir"
  exit 1
}
if { ! [regexp {/data$} $bdj3dir] } {
  append bdj3dir /data
}
if { ! [file exists $bdj3dir] || ! [file isdirectory $bdj3dir] } {
  puts "Invalid directory $bdj3dir"
  exit 1
}
set datatopdir [lindex $argv 1]
if { ! [file exists $datatopdir/data] || ! [file isdirectory $datatopdir/data] } {
  puts "Invalid directory $datatopdir"
  exit 1
}

if { $::tcl_platform(platform) eq "windows" } {
  set hostname [info hostname]
} else {
  set hostname [exec hostname]
}

set suffixlist [list .txt]
set nprefixlist [list profile00/]
for { set i 1 } { $i < 20 } { incr i } {
  lappend suffixlist -$i.txt
  if { $i < 10 } {
    lappend nprefixlist "profile0${i}/"
  } else {
    lappend nprefixlist "profile${i}/"
  }
}

set cnm bdj_config
set nnm bdjconfig
set mpath $hostname
set mppath [file join $hostname profiles]
set col {}
foreach path [list {} profiles $mpath $mppath] {
  foreach sfx $suffixlist pfx $nprefixlist {
    if { $path eq $mpath && $sfx ne {.txt} } {
      # these are really old filenames from old versions of ballroomdj
      # when profiles were in a different place.
      # do not process these.
      continue
    }

    set fn "[file join $bdj3dir $path $cnm]$sfx"
    if { [file exists $fn] } {
      set olddirlist [list]
      set musicdir {}
      set tdir $path
      if { [regexp {profiles} $path] } {
        set tdir [file dirname $path]
      } else {
        set pfx {}
      }

      set nfn "[file join $datatopdir data $tdir $pfx $nnm].txt"

      set ifh [open $fn r]
      file mkdir [file dirname $nfn]
      set imgdir {}
      if { $tdir eq {.} && $pfx ne {} } {
        set imgdir [file join $datatopdir img $pfx]
        file mkdir $imgdir
        if { $imgdir ne {} && $col ne {} } {
          copyimages $imgdir $col
        }
      }
      set ofh [open $nfn w]
      if { $path eq "profiles" } {
        foreach q {0 1 2 3} {
          set tfn "[file join $datatopdir data $tdir $pfx $nnm.q$q].txt"
          set tfh [open $tfn w]
          set qofh$q $tfh
          puts $tfh "# config-queue"
          puts $tfh "# [clock format [clock seconds] -gmt 1 -format {%Y-%m-%d %H:%M:%S}]"
          puts $tfh "version"
          puts $tfh "..1"
          puts $tfh "ACTIVE"
          if { $q == 0 } {
            puts $tfh "..yes"
          } else {
            puts $tfh "..no"
          }
          puts $tfh "DISPLAY"
          if { $q == 0 || $q == 1 } {
            puts $tfh "..yes"
          } else {
            puts $tfh "..no"
          }
          puts $tfh "PAUSEEACHSONG"
          puts $tfh "..no"
          puts $tfh "PLAYANNOUNCE"
          puts $tfh "..no"
          puts $tfh "PLAY_WHEN_QUEUED"
          puts $tfh "..no"
          puts $tfh "SHOWQUEUEDANCE"
          puts $tfh "..no"
          puts $tfh STOP_AT_TIME
          puts $tfh "..0"
          if { $q == 2 } {
            puts $tfh "QUEUE_NAME"
            puts $tfh "..Queue C"
          }
          if { $q == 3 } {
            puts $tfh "QUEUE_NAME"
            puts $tfh "..Queue D"
          }
        }
      }

      if { $path eq {} } {
        puts $ofh "# config-global"
      }
      if { $path eq "profiles" } {
        puts $ofh "# config-profile"
      }
      if { $path eq $mpath } {
        puts $ofh "# config-machine"
      }
      if { $path eq $mppath } {
        puts $ofh "# config-machine-profile"
      }
      puts $ofh "# [clock format [clock seconds] -gmt 1 -format {%Y-%m-%d %H:%M:%S}]"
      puts $ofh "version"
      if { $path eq {} } {
        puts $ofh "..3"
      }
      if { $path eq "profiles" } {
        puts $ofh "..3"
      }
      if { $path eq $mpath } {
        puts $ofh "..1"
      }
      if { $path eq $mppath } {
        puts $ofh "..1"
      }

      while { [gets $ifh line] >= 0 } {
        if { [regexp {^#} $line] } {
          continue
        }
        regexp {^([^:]*):(.*)$} $line all key value

        if { $key eq "ACOUSTID_CLIENT" } { continue }
        if { $key eq "ALLOWEDIT" } { continue }
        if { $key eq "AUTOSTARTUP" } { continue }
        if { $key eq "CBFONTSIZE" } { continue }
        # debug level should be in the global; so just remove it.
        if { $key eq "DEBUGLVL" } { continue }
        if { $key eq "DEBUGON" } { continue }
        if { $key eq "ENABLEIMGPLAYER" } { continue }
        if { $key eq "FONTSIZE" } { continue }
        if { $key eq "HOST" } { continue }
        if { $key eq "INSTPASSWORD" } { continue }
        if { $key eq "ITUNESSUPPORT" } { continue }
        if { $key eq "MQCLOCKFONTSIZE" } { continue }
        if { $key eq "MQDANCEFONT" } { continue }
        if { $key eq "MQDANCEFONTMULT" } { continue }
        if { $key eq "MQDANCELOC" } { continue }
        if { $key eq "MQFULLSCREEN" } { continue }
        if { $key eq "MQPROGBARCOLOR" } { continue }
        if { $key eq "MQSHOWBUTTONS" } { continue }
        if { $key eq "MQSHOWCLOCK" } { continue }
        if { $key eq "MQSHOWCOUNTDOWN" } { continue }
        if { $key eq "MQSHOWPROGBAR" } { continue }
        if { $key eq "MQSHOWTITLE" } { continue }
        if { $key eq "NATIVEFILEDIALOGS" } { continue }
        if { $key eq "PLAYERQLEN1" } { continue }
        if { $key eq "QUICKPLAYENABLED" } { continue }
        if { $key eq "QUICKPLAYSHOW" } { continue }
        if { $key eq "REMCONTROLSHOWDANCE" } { continue }
        if { $key eq "REMCONTROLSHOWSONG" } { continue }
        if { $key eq "SCALEDWIDGETS" } { continue }
        if { $key eq "SERVERNAME" } { continue }
        if { $key eq "SERVERPASS" } { continue }
        if { $key eq "SERVERPORT" } { continue }
        if { $key eq "SERVERTYPE" } { continue }
        if { $key eq "SERVERUSER" } { continue }
        if { $key eq "SHOWSTATUS" } { continue }
        if { $key eq "SLOWDEVICE" } { continue }
        if { $key eq "STARTMAXIMIZED" } { continue }
        if { $key eq "SYNCROLE" } { continue }
        if { $key eq "UIFIXEDFONT" } { continue }
        if { $key eq "WEBENABLE" } { continue }
        if { $key eq "WEBPASS" } { continue }
        if { $key eq "WEBPORT" } { continue }
        if { $key eq "WEBUSER" } { continue }
        if { [regexp {^[a-z_]} $key] } { continue }
        if { [regexp {^KEY} $key] } { continue }
        if { [regexp {^MQCOL} $key] } { continue }
        if { [regexp {^UI.*COLOR$} $key] } { continue }
        # renamed; moved to MP
        if { $key eq "AUDIOSINK" } {
          # audiosink is moved from machine to machine-profile
          set audiosink $value
          if { $audiosink eq "" } {
            set audiosink default
          }
          continue
        }
        if { $key eq "UITHEME" } { continue }
        # moved to M
        if { $key eq "PLAYER" } { continue }
        if { $key eq "IMAGEDIR" } { continue }
        if { $key eq "MTMPDIR" } { continue }
        if { $key eq "CLVAPATHFMT" } { continue }
        if { $key eq "VAPATHFMT" } { continue }
        if { $key eq "CLPATHFMT" } { continue }
        if { $key eq "SHOWALBUM" } { continue }
        if { $key eq "SHOWCLASSICAL" } { continue }
        if { $key eq "VARIOUS" } { continue }
        if { $key eq "PAUSEMSG" } { continue }
        if { $key eq "CHANGESPACE" } { continue }
        if { $key eq "MUSICDIRDFLT" } { continue }
        if { $key eq "MOBILEMQTAG" } { continue }
        if { $key eq "version" } { continue }

        if { $key eq "DONEMSG" } { set key "COMPLETEMSG" }
        if { $key eq "SHOWBPM" } { set key "BPM" }
        if { $key eq "PLAYERQLEN0" } { set key "PLAYERQLEN" }
        if { $key eq "PATHFMT" } { set key "ORGPATH" }
        if { $key eq "MOBILEMARQUEE" } { set key "MOBILEMQTYPE" }
        if { $key eq "MUSICDIR" } {
          set key DIRMUSIC
          set musicdir $value
        }

        if { $key eq "ORIGINALDIR" ||
            $key eq "DELETEDIR" ||
            $key eq "ARCHIVEDIR" } {
          # check and see if these folders are within the music-dir
          lappend olddirlist $value
          continue
        }

        # force these off so that the BDJ3 files will not be affected.
        if { $key eq "WRITETAGS" } { set value NONE }
        if { $key eq "AUTOORGANIZE" } { set value no }
        if { $key eq "UIFONT" } {
          regsub -all "\{" $value {} value
          regsub -all "\}" $value {} value
        }
        if { $key eq "LISTINGFONTSIZE" } {
          set key LISTINGFONT
          set value {}
          if { $::tcl_platform(os) eq "Linux" } {
            set value [exec gsettings get org.gnome.desktop.interface font-name]
            regsub -all {'} $value {} value
          }
          if { $::tcl_platform(platform) eq "windows" } {
            set value "Arial Regular 13"
          }
          if { $::tcl_platform(os) eq "Darwin" } {
            set value "Arial Regular 16"
          }
        }

        if { $key eq "MQSHOWARTIST" } {
          set key MQSHOWINFO
        }
        if { $key eq "QUEUENAME0" } {
          set key QUEUE_NAME_A
          if { $value eq "Queue 1" } {
            set value {Music Queue}
          }
          if { $value eq "Wachtrij 1" } {
            set value {Muziek Wachtrij}
          }
        }
        if { $key eq "QUEUENAME1" } {
          set key QUEUE_NAME_B
          if { $value eq "Queue 2" } {
            set value {Queue B}
          }
          if { $value eq "Wachtrij 2" } {
            set value {Wachtrij B}
          }
        }
        if { $key eq "BPM" && $value eq "OFF" } {
          set value BPM
        }
        if { $key eq "PLAYERQLEN" } {
          set value 90
        }
        if { $key eq "PROFILENAME" && $value eq {BallroomDJ} } {
          set value {BallroomDJ 4}
        }
        if { $key eq "FADEINTIME" && $value eq {} } {
          set value 0
        }
        if { $key eq "FADEOUTTIME" && $value eq {} } {
          set value 0
        }
        if { $key eq "UIFONT" && $value eq {} } {
          if { $::tcl_platform(os) eq "Linux" } {
            set value [exec gsettings get org.gnome.desktop.interface font-name]
            regsub -all {'} $value {} value
          }
          if { $::tcl_platform(platform) eq "windows" } {
            set value "Arial Regular 14"
          }
          if { $::tcl_platform(os) eq "Darwin" } {
            set value "Arial Regular 17"
          }
        }
        if { $key eq "MQFONT" && $value ne {} } {
          set value [join $value { }]
        }
        if { $key eq "MQFONT" && $value eq {} } {
          if { $::tcl_platform(platform) eq "windows" } {
            # windows has no narrow fonts installed by default
            set value "Arial Regular 14"
          }
          if { $::tcl_platform(os) eq "Darwin" } {
            set value "Arial Narrow Regular 17"
          }
        }
        if { $key eq "GAP" && $value eq {} } {
          set value 0
        }
        if { $key eq "SHUTDOWNSCRIPT" || $key eq "STARTUPSCRIPT" } {
          if { [regexp {linux/bdj(startup|shutdown)\.sh$} $value] } {
            regsub {.*linux/} $value "${topdir}/scripts/linux/" value
          }
        }
        if { $key eq "FADEINTIME" || $key eq "FADEOUTTIME" ||
            $key eq "GAP" } {
          set value [expr {int ($value * 1000)}]
        }
        if { $key eq "MAXPLAYTIME" } {
          if { $value ne {} } {
            regexp {(\d+):(\d+)} $value all min sec
            regsub {^0*} $min {} min
            if { $min eq {} } { set min 0 }
            regsub {^0*} $sec {} sec
            if { $sec eq {} } { set sec 0 }
            set value [expr {($min * 60 + $sec) * 1000}]
          }
          if { $value eq {} } {
            set value 360000
          }
        }
        if { [regexp {ORGPATH$} $key] } {
          # various is no longer supported, remove the group entirely.
          regsub -all "{\[^A-Z\]*PVARIOUS\[^A-Z\]*}" $value {} value
          # keyword is no longer supported, remove the group entirely.
          regsub -all "{\[^A-Z\]*PKEYWORD\[^A-Z\]*}" $value {} value
          regsub -all {PALBART} $value {%ALBUMARTIST%} value
          regsub -all {PDANCERATING} $value {%RATING%} value
          regsub -all {PTRACKNUM} $value {PTRACKNUMBER} value
          if { [regexp {PDANCE.*PDANCE} $key] } {
            # fix a duplication error that was seen
            # note that bdj3 would have stripped off the first PDANCE
            # and matched via the second PDANCE.
            regsub "{PDANCE/}" $value {} value
          }
          regsub -all {P([A-Z][A-Z]*0?)} $value {%\1%} value
        }

        if { $key eq "FADEINTIME" || $key eq "FADEOUTTIME" ||
            $key eq "GAP" || $key eq "MAXPLAYTIME" } {
          foreach q {0 1 2 3} {
            set tfh [set qofh$q]
            puts $tfh $key
            puts $tfh "..$value"
          }
        } elseif { $key eq "QUEUE_NAME_A" } {
          puts $qofh0 QUEUE_NAME
          puts $qofh0 "..$value"
        } elseif { $key eq "QUEUE_NAME_B" } {
          puts $qofh1 QUEUE_NAME
          puts $qofh1 "..$value"
        } else {
          puts $ofh $key
          puts $ofh "..$value"
        }
      }

      if { $path eq {} } {
        puts $ofh ACOUSTID_KEY
        puts $ofh "..ENCTFdHUmEcJxkaKAAABRY="
        puts $ofh "ACRCLOUD_API_KEY"
        puts $ofh ".."
        puts $ofh "ACRCLOUD_API_SECRET"
        puts $ofh ".."
        puts $ofh "ACRCLOUD_API_HOST"
        puts $ofh ".."
        puts $ofh CLOCKDISP
        puts $ofh "..local"
        puts $ofh DEBUGLVL
        if { $::tcl_platform(os) eq "Darwin" } {
          # logging on macos is really slow
          puts $ofh "..0"
        } else {
          puts $ofh "..11"
        }
      }
      if { $path eq "profiles" } {
        puts $ofh MARQUEE_SHOW
        puts $ofh "..visible"
        puts $ofh MQ_ACCENT_COL
        puts $ofh "..#030e80"
        puts $ofh MQ_INFO_COL
        puts $ofh "..#444444"
        puts $ofh MQ_INFO_SEP
        puts $ofh "../"
        puts $ofh MQ_TEXT_COL
        puts $ofh "..#000000"
        puts $ofh UI_ACCENT_COL
        puts $ofh "..#ffa600"
        puts $ofh UI_ERROR_COL
        puts $ofh "..#ff2222"
        puts $ofh UI_MARK_COL
        puts $ofh "..#2222ff"

        set col "#ffa600"
        if { $sfx ne ".txt" } {
          set col [mkrandcolor]
        }
        puts $ofh UI_PROFILE_COL
        puts $ofh "..${col}"
      }
      if { $path eq $mpath } {
        puts $ofh AUDIOTAG
        puts $ofh "..libatibdj4"
        puts $ofh VOLUME
        if { $::tcl_platform(os) eq "Linux" } { set value libvolpa }
        if { $::tcl_platform(platform) eq "windows" } { set value libvolwin }
        if { $::tcl_platform(os) eq "Darwin" } { set value libvolmac }
        puts $ofh "..$value"

        set value libplivlc
        puts $ofh PLAYER
        puts $ofh "..$value"

        set value "Integrated VLC"
        puts $ofh PLAYER_I_NM
        puts $ofh "..$value"

        set value {}
        set tfn [file join $musicdir iTunes {iTunes Media}]
        if { [file isdirectory $tfn] } {
          set value $tfn
        }
        puts $ofh DIRITUNESMEDIA
        puts $ofh "..$value"

        if { $value ne {} } {
          set value {}
          set tfn [file normalize [file join $tfn .. {iTunes Music Library.xml}]]
          if { [file exists $tfn] } {
            set value $tfn
          }
        }
        puts $ofh ITUNESXMLFILE
        puts $ofh "..$value"
      }
      if { $path eq $mppath } {
        # audiosink is moved from machine to machine-profile
        puts $ofh AUDIOSINK
        puts $ofh "..${audiosink}"

        puts $ofh MQ_THEME
        set value Adwaita
        if { $::tcl_platform(os) eq "Linux" } { set value Adwaita }
        if { $::tcl_platform(platform) eq "windows" } { set value Windows-10 }
        if { $::tcl_platform(os) eq "Darwin" } { set value MacOS }
        puts $ofh "..${value}"

        puts $ofh UI_THEME
        set value Adwaita  ; # just something as a default
        if { $::tcl_platform(os) eq "Linux" } {
          # use the dark adwaita
          set value Adwaita-dark
        }
        if { $::tcl_platform(platform) eq "windows" } { set value Windows-10-Dark }
        if { $::tcl_platform(os) eq "Darwin" } { set value Mojave-dark-solid }
        puts $ofh "..${value}"
        set tfh [open [file join $datatopdir data theme.txt] w]
        puts $tfh "${value}"
        close $tfh
      }

      if { $musicdir ne {} && [llength $olddirlist] > 0 } {
        set skiplist [list]

        foreach d $olddirlist {
          if { [regexp "^${musicdir}" $d] } {
            try {
              set flist [glob -directory $d -type f *]
            } on error {err res} {
              set flist [list]
            }
            if { [llength $flist] > 0 } {
              regsub "^${musicdir}/" $d {} tdir
              lappend skiplist $tdir
            }
          }
        }

        if { [llength $skiplist] > 0 } {
          puts $ofh "DIROLDSKIP"
          set tmp [join $skiplist ;]
# puts "== skiplist: $tmp"
          puts $ofh "..${tmp}"
        }
      }

      close $ifh
      close $ofh
      if { $path eq "profiles" } {
        foreach q {0 1 2} {
          set tfh [set qofh$q]
          close $tfh
        }
      }
    }
  }
}
