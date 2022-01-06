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

set flist [glob -directory $dir *.playlist]

foreach {fn} $flist {
  puts "## Converting: [file tail $fn]"
  set ifh [open $fn r]
  set nfn [file join data [file rootname [file tail $fn]].pl]
  set dfn [file join data [file rootname [file tail $fn]].pldances]
  set ofh [open $nfn w]
  set dfh [open $dfn w]
  puts $ofh "# BDJ4 playlist settings"
  puts $ofh "# Converted from $fn"
  puts $ofh "# [clock format [clock seconds] -gmt 1]"
  puts $dfh "# BDJ4 playlist dances"
  puts $dfh "# Converted from $fn"
  puts $dfh "# [clock format [clock seconds] -gmt 1]"
  puts $dfh "version"
  puts $dfh "..1"
  set pltype Automatic
  set keyidx 0
  while { [gets $ifh line] >= 0 } {
    regexp {^([^:]*):(.*)$} $line all key value
    if { $key eq "list" } { continue }
    if { $key eq "versoin"} { continue }

    if { $key eq "playannounce" } { set key PlayAnnounce }
    if { $key eq "maxplaytime" } { set key MaxPlayTime }
    if { $key eq "pauseeachsong" } { set key PauseEachSong }
    if { $key eq "resume" } { set key Resume }
    if { $key eq "stopafter" } { set key StopAfter }
    if { $key eq "stopwait" } { set key StopWait }
    if { $key eq "stopafterwait" } { set key StopAfterWait }
    if { $key eq "stoptime" } { set key StopTime }
    if { $key eq "stoptype" } { set key StopType }
    if { $key eq "StatusOK" } { set key UseStatus }
    if { $key eq "UnratedOK" } { set key UseUnrated }
    if { $key eq "gap" } { set key Gap }
    if { $key eq "highDanceLevel" } { set key HighDanceLevel }
    if { $key eq "lowDanceLevel" } { set key LowDanceLevel }
    if { $key eq "mqMessage" } { set key MqMessage }
    if { $key eq "ManualList" && $value eq "None" } {
      set value {}
    }
    if { $key eq "ManualList" && $value ne {} } {
      set pltype Manual
    }
    if { $key eq "Sequence" && $value eq "None" } {
      set value {}
    }
    if { $key eq "Sequence" && $value ne {} } {
      set pltype Sequence
    }
    if { $key eq "VERSION" } { set key version }

    if { [regexp {:\d+:} $value] } {
      puts $dfh KEY
      puts $dfh "..$keyidx"
      puts $dfh DANCE
      puts $dfh "..$key"
      regexp {(\d):(\d*):(\d*):(\d*):(\d*):} $value \
          all selected count maxmin maxsec lowbpm highbpm
      puts $dfh "SELECTED"
      puts $dfh "..$selected"
      puts $dfh "COUNT"
      puts $dfh "..$count"
      puts $dfh "MAXPLAYTIME"
      set tm {}
      if { $maxmin ne {} && $maxsec ne {} } {
        set tm "$maxmin:$maxsec"
      }
      puts $dfh "..$tm"
      puts $dfh "LOWBPM"
      puts $dfh "..$lowbpm"
      puts $dfh "HIGHBPM"
      puts $dfh "..$highbpm"
      incr keyidx
    } else {
      set key [string toupper $key]
      puts $ofh $key
      puts $ofh "..$value"
    }
  }
  puts $ofh "TYPE"
  puts $ofh "..$pltype"
  close $ifh
  close $ofh
}
