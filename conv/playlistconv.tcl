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

    set tkey [string toupper $key]
    if { $tkey eq "REQUIREDKEYWORDS" } { continue }
    if { $tkey eq "STOPTIME" } { continue }
    if { $tkey eq "STOPWAIT" } { continue }
    if { $tkey eq "STOPAFTER" } { continue }
    if { $tkey eq "STOPAFTERWAIT" } { continue }
    if { $tkey eq "STOPTYPE" } { continue }
    if { $tkey eq "RESUME" } { continue }

    if { $tkey eq "STATUSOK" } { set key USESTATUS }
    if { $tkey eq "UNRATEDOK" } { set key USEUNRATED }
    if { $tkey eq "HIGHDANCELEVEL" } { set key DANCELEVELHIGH }
    if { $tkey eq "LOWDANCELEVEL" } { set key DANCELEVELLOW }

    if { $tkey eq "MANUALLIST" && $value eq "None" } {
      set value {}
    }
    if { $tkey eq "MANUALLIST" && $value ne {} } {
      set pltype Manual
    }
    if { $tkey eq "SEQUENCE" && $value eq "None" } {
      set value {}
    }
    if { $tkey eq "GAP" && $value eq {} } {
      set value -1
    }
    if { $tkey eq "MAXPLAYTIME" && $value eq {} } {
      set value -1
    }
    if { $tkey eq "SEQUENCE" && $value ne {} } {
      set pltype Sequence
    }
    if { $tkey eq "VERSION" } { set key version }

    if { [regexp {:\d+:} $value] } {
      puts $dfh KEY
      puts $dfh "..$keyidx"
      puts $dfh DANCE
      puts $dfh "..$key"
      regexp {(\d):(\d*):(\d*):(\d*):(\d*):(\d*)} $value \
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
      puts $dfh "BPMLOW"
      puts $dfh "..$lowbpm"
      puts $dfh "BPMHIGH"
      puts $dfh "..$highbpm"
      incr keyidx
    } else {
      if { $tkey eq "MAXPLAYTIME" } {
        if { $value ne {} && $value ne "-1" } {
          regexp {(\d+):(\d+)} $value all min sec
          set value [expr {($min * 60 + $sec)*1000}]
        }
      }
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
