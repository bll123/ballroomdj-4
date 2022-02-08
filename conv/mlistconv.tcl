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

puts "-- Converting: song lists"
foreach {fn} $flist {
  set nfn [file join data [file rootname [file tail $fn]].songlist]
  puts "    - [file tail $fn] : $nfn"
  source $fn
  set fh [open $nfn w]
  puts $fh "# BDJ4 songlist"
  puts $fh "# Converted from $fn"
  puts $fh "# [clock format [clock seconds] -gmt 1]"
  puts $fh version
  puts $fh "..1"
  puts $fh count
  puts $fh "..$slcount"
  foreach {key data} $sllist {
    puts $fh "KEY\n..$key"
    foreach {k v} $data {
      if { $k eq "DANCE" } {
        regsub {\s\([^)]*\)$} $v {} v
      }
      puts $fh $k
      puts $fh "..$v"
    }
  }
  close $fh
}
