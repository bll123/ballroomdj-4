#!/usr/bin/tclsh

if { $argc != 2 } {
  puts "usage: $argv0 <bdj3dir> <datatopdir>"
  exit 1
}

set bdj3dir [lindex $argv 0]
if { ! [file exists $bdj3dir] || ! [file isdirectory $bdj3dir] } {
  puts "Invalid directory $bdj3dir"
  exit 1
}
set datatopdir [lindex $argv 1]

set flist [glob -directory $bdj3dir *.mlist]

puts "-- Converting: song lists"
foreach {fn} $flist {
  set nfn [file join $datatopdir data [file rootname [file tail $fn]].songlist]
  puts "    - [file tail $fn] : [file rootname [file tail $fn]].songlist"
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
