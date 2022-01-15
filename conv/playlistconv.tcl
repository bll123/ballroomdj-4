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
    if { $key eq "version"} { continue }

    set key [string toupper $key]
    if { $key eq "REQUIREDKEYWORDS" } { continue }
    if { $key eq "STOPTIME" } { continue }
    if { $key eq "STOPWAIT" } { continue }
    if { $key eq "STOPAFTER" } { continue }
    if { $key eq "STOPAFTERWAIT" } { continue }
    if { $key eq "STOPTYPE" } { continue }
    if { $key eq "RESUME" } { continue }

    if { $key eq "STATUSOK" } { set key USESTATUS }
    if { $key eq "UNRATEDOK" } { set key USEUNRATED }
    if { $key eq "HIGHDANCELEVEL" } { set key DANCELEVELHIGH }
    if { $key eq "LOWDANCELEVEL" } { set key DANCELEVELLOW }

    if { $key eq "MANUALLIST" && $value eq "None" } {
      set value {}
    }
    if { $key eq "MANUALLIST" && $value ne {} } {
      set pltype Manual
    }
    if { $key eq "SEQUENCE" && $value eq "None" } {
      set value {}
    }
    if { $key eq "GAP" && $value eq {} } {
      set value -1
    }
    if { $key eq "MAXPLAYTIME" && $value eq {} } {
      set value -1
    }
    if { $key eq "SEQUENCE" && $value ne {} } {
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
        set tm [expr {($maxmin * 60 + $maxsec)*1000}]
      }
      puts $dfh "..$tm"
      puts $dfh "LOWBPM"
      puts $dfh "..$lowbpm"
      puts $dfh "HIGHBPM"
      puts $dfh "..$highbpm"
      incr keyidx
    } else {
      if { $key eq "MAXPLAYTIME" } {
        if { $value ne {} && $value ne "-1" } {
          regexp {(\d+):(\d+)} $value all min sec
          set value [expr {($min * 60 + $sec)*1000}]
        }
      }
      puts $ofh $key
      puts $ofh "..$value"
    }
  }
  puts $ofh "TYPE"
  puts $ofh "..$pltype"
  close $ifh
  close $ofh
}
