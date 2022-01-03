#!/usr/bin/tclsh

if { $argc <= 0 } {
  puts "usage: $argv0 <directory>"
  exit 1
}

set dir [lindex $argv 0]
if { ! [file exists $dir] || ! [file isdirectory $dir] } {
  puts "Invalid directory $dir"
  exit 1
}

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
foreach path [list {} profiles $hostname [file join $hostname profiles]] {
  foreach sfx $suffixlist {
    set fn "[file join $dir $path $cnm]$sfx"
    if { [file exists $fn] } {
      puts "## Converting: [file join $path $cnm]$sfx"
      set ifh [open $fn r]
      set ofh [open "[file join data $path $nnm]$sfx" w]

      puts $ofh "# BDJ4 [file join $path $nnm]"
      puts $ofh "# [clock format [clock seconds] -gmt 1]"

      while { [gets $ifh line] >= 0 } {
        if { [regexp {^#} $line] } {
          puts $ofh $line
          continue
        }
        regexp {^([^:]*):(.*)$} $line all key value
        if { $key eq "SYNCROLE" } {
          # synchronization will not be implemented in version 4.
          continue
        }
        if { [regexp {^[a-z_]} $key] } {
          continue
        }
        if { [regexp {^MQCOL} $key] } {
          continue
        }
        if { [regexp {^UI.*COLOR$} $key] } {
          continue
        }
        if { [regexp {^KEY} $key] } {
          continue
        }
        if { $key eq "DEBUGON" } { continue }
        if { $key eq "MQDANCELOC" } { continue }
        if { $key eq "MQPROGBARCOLOR" } { continue }
        if { $key eq "MQSHOWARTIST" } { continue }
        if { $key eq "MQSHOWBUTTONS" } { continue }
        if { $key eq "MQSHOWCLOCK" } { continue }
        if { $key eq "MQSHOWCOUNTDOWN" } { continue }
        if { $key eq "MQSHOWPROGBAR" } { continue }
        if { $key eq "MQSHOWTITLE" } { continue }
        if { $key eq "CBFONTSIZE" } { continue }
        if { $key eq "MQCLOCKFONTSIZE" } { continue }
        if { $key eq "MQDANCEFONT" } { continue }
        if { $key eq "INSTPASSWORD" } { continue }
        if { $key eq "NATIVEFILEDIALOGS" } { continue }
        if { $key eq "SCALEDWIDGETS" } { continue }

        if { $key eq "UITHEME" } { set value {} }
        if { $key eq "version" } { set value 10 }
        if { $key eq "MUSICDIR" } { set key DIRMUSIC }
        if { $key eq "ORIGINALDIR" } { set key DIRORIGINAL }
        if { $key eq "DELETEDIR" } { set key DIRDELETE }
        if { $key eq "MTMPDIR" } { set key DIRMUSICTMP }
        if { $key eq "IMAGEDIR" } { set key DIRIMAGE }
        if { $key eq "ARCHIVEDIR" } { set key DIRARCHIVE }
        puts $ofh $key
        puts $ofh "..$value"
      }

      close $ifh
      close $ofh
    }
  }
}


