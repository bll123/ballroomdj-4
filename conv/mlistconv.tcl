#!/usr/bin/tclsh

if { $argc <= 0 } {
  puts "usage: mlistconv <directory>"
  exit 1
}

set flist [glob -directory [lindex $argv 0] *.mlist]

foreach {fn} $flist {
  source $fn
  set nfn [file rootname $fn].songlist
  set fh [open $nfn w]
  puts $fh "# BDJ4 songlist"
  puts $fh "# Converted from $fn"
  puts $fh "# [clock format [clock seconds] -gmt 1]"
  puts $fh version
  puts $fh "..10"
  puts $fh count
  puts $fh "..$slcount"
  foreach {key data} $sllist {
    puts $fh "KEY\n..$key"
    foreach {k v} $data {
      puts $fh $k
      puts $fh "..$v"
    }
  }
  close $fh
}
