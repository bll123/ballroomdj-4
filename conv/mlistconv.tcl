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

set flist [glob -directory $dir *.mlist]

foreach {fn} $flist {
  puts "## Converting: [file tail $fn]"
  source $fn
  set nfn [file join data [file rootname [file tail $fn]].songlist]
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
