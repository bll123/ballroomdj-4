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
if { ! [regexp {/data$} $bdj3dir] } {
  append bdj3dir /data
}
if { ! [file exists $bdj3dir] || ! [file isdirectory $bdj3dir] } {
  puts "Invalid directory $bdj3dir"
  exit 1
}
set datatopdir [lindex $argv 1]
if { ! [file exists $datatopdir/data] || ! [file isdirectory $datatopdir/data] } {
  puts "Invalid directory $datatopdir"
  exit 1
}

try {
  set flist [glob -directory $bdj3dir *.mlist]
} on error { err res } {
  set flist {}
}

foreach {fn} $flist {
  if { [file rootname [file tail $fn]] eq "Raffle Songs" } {
    # raffle games are no longer needed
    continue;
  }

  set nfn [file join $datatopdir data [file rootname [file tail $fn]].songlist]
#  puts "   - [file tail $fn] : [file rootname [file tail $fn]].songlist"
  source $fn
  set fh [open $nfn w]
  puts $fh "# songlist"
  puts $fh "# Converted from $fn"
  puts $fh "# [clock format [clock seconds] -gmt 1 -format {%Y-%m-%d %H:%M:%S}]"
  puts $fh version
  puts $fh "..1"
  puts $fh count
  puts $fh "..$slcount"
  foreach {key data} $sllist {
    puts $fh "KEY\n..$key"
    foreach {k v} $data {
      if { $k eq "FILE" } {
        set k URI
      }
      if { $k eq "DANCE" } {
        regsub {\s\([^)]*\)$} $v {} v
      }
      puts $fh $k
      puts $fh "..$v"
    }
  }
  close $fh
}
