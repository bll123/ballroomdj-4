#!/usr/bin/tclsh

if { $argc <= 0 } {
  puts "usage: playlistconv <directory>"
  exit 1
}

set flist [glob -directory [lindex $argv 0] *.playlist]

foreach {fn} $flist {
  set ifh [open $fn r]
  set nfn [file rootname $fn].playlist4
  set ofh [open $nfn w]
  puts $ofh "# BDJ4 playlist"
  puts $ofh "# Converted from $fn"
  puts $ofh "# [clock format [clock seconds] -gmt 1]"
  while { [gets $ifh line] >= 0 } {
    regexp {^([^:]*):(.*)$} $line all key value
    if { $key eq "version" } {
      set value 10
    }
    puts $ofh $key
    puts $ofh "..$value"
  }
  close $ifh
  close $ofh
}
