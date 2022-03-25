#!/usr/bin/tclsh

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
set datatopdir [lindex $argv 1]

if { $::tcl_platform(platform) eq "windows" } {
  set hostname [info hostname]
} else {
  set hostname [exec hostname]
}

set suffixlist [list .txt]
for { set i 1 } { $i < 20 } { incr i } {
  lappend suffixlist -$i.txt
}

set cnm bdj_config
set nnm bdjconfig
set mpath $hostname
set mppath [file join $hostname profiles]
foreach path [list {} profiles $mpath $mppath] {
  foreach sfx $suffixlist {
    set fn "[file join $bdj3dir $path $cnm]$sfx"
    if { [file exists $fn] } {
      set nfn "[file join $datatopdir data $path $nnm]$sfx"
#      puts "   - [file join $path $cnm]$sfx : [file join $path $nnm]$sfx"
      set ifh [open $fn r]
      file mkdir [file join $datatopdir data $path]
      set ofh [open $nfn w]

      puts $ofh "# BDJ4 [file join $path $nnm]"
      puts $ofh "# [clock format [clock seconds] -gmt 1]"

      while { [gets $ifh line] >= 0 } {
        if { [regexp {^#} $line] } {
          puts $ofh $line
          continue
        }
        regexp {^([^:]*):(.*)$} $line all key value

        if { $key eq "ACOUSTID_CLIENT" } { continue }
        if { $key eq "AUTOSTARTUP" } { continue }
        if { $key eq "CBFONTSIZE" } { continue }
        # debug level should be in the global; so just remove it.
        if { $key eq "DEBUGLVL" } { continue }
        if { $key eq "DEBUGON" } { continue }
        if { $key eq "ENABLEIMGPLAYER" } { continue }
        if { $key eq "FONTSIZE" } { continue }
        if { $key eq "INSTPASSWORD" } { continue }
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
        if { $key eq "STARTMAXIMIZED" } { continue }
        if { $key eq "SYNCROLE" } { continue }
        if { $key eq "UIFIXEDFONT" } { continue }
        if { [regexp {^[a-z_]} $key] } { continue }
        if { [regexp {^KEY} $key] } { continue }
        if { [regexp {^MQCOL} $key] } { continue }
        if { [regexp {^UI.*COLOR$} $key] } { continue }
        # renamed; moved to MP
        if { $key eq "UITHEME" } { continue }
        # moved to M
        if { $key eq "PLAYER" } { continue }
        if { $key eq "ORIGINALDIR" } { continue }
        if { $key eq "DELETEDIR" } { continue }
        if { $key eq "IMAGEDIR" } { continue }
        if { $key eq "ARCHIVEDIR" } { continue }
        if { $key eq "MTMPDIR" } { continue }

        if { $key eq "version" } { set value 1 }

        if { $key eq "PLAYERQLEN0" } { set key "PLAYERQLEN" }
        if { $key eq "CLPATHFMT" } { set key PATHFMT_CL }
        if { $key eq "CLVAPATHFMT" } { set key PATHFMT_CLVA }
        if { $key eq "VAPATHFMT" } { set key PATHFMT_VA }
        if { $key eq "MUSICDIR" } { set key DIRMUSIC }
        if { $key eq "LISTINGFONTSIZE" } {
          set key LISTINGFONT
          set value {}
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
        if { $key eq "PLAYERQLEN" } {
          if { $value < 10 } {
            set value 30
          }
        }
        if { $key eq "MQFONT" && $value ne {} } {
          # drop any size and remove the braces.
          set value [lindex $value 0]
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
          if { $::tcl_platform(platform) eq "windows" } {
            set value "Arial 11"
          }
        }
        if { $key eq "MQFONT" && $value eq {} } {
          if { $::tcl_platform(platform) eq "windows" } {
            set value "Arial 11"
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

        puts $ofh $key
        puts $ofh "..$value"
      }

      if { $path eq "profiles" } {
        puts $ofh INSERT_LOC
        puts $ofh "..6"
        puts $ofh MQ_ACCENT_COL
        puts $ofh "..#030e80"
        puts $ofh UI_ACCENT_COL
        puts $ofh "..#ffa600"
        puts $ofh HIDEMARQUEEONSTART
        puts $ofh "..1"
      }
      if { $path eq $mpath } {
        puts $ofh VOLUME
        if { $::tcl_platform(os) eq "Linux" } { set value libvolpa }
        if { $::tcl_platform(platform) eq "windows" } { set value libvolwin }
        if { $::tcl_platform(os) eq "Darwin" } { set value libvolmac }
        puts $ofh "..$value"
        puts $ofh PLAYER
        set value libplivlc
        puts $ofh "..$value"
      }
      if { $path eq $mppath } {
        puts $ofh MQ_THEME
        set value Adwaita
        if { $::tcl_platform(os) eq "Linux" } { set value Adwaita }
        if { $::tcl_platform(platform) eq "windows" } { set value Windows-10 }
        if { $::tcl_platform(os) eq "Darwin" } { set value MacOS }
        puts $ofh "..${value}"

        puts $ofh UI_THEME
        set value Adwaita-Dark
        if { $::tcl_platform(os) eq "Linux" } { set value Adwaita-Dark }
        if { $::tcl_platform(platform) eq "windows" } { set value Windows-10-Dark }
        if { $::tcl_platform(os) eq "Darwin" } { set value MacOS-Dark }
        puts $ofh "..${value}"
      }
      close $ifh
      close $ofh
    }
  }
}


