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

set fn autosel.txt
set infn [file join $bdj3dir $fn]
if { ! [file exists $infn] } {
  puts "   no auto selection file"
  exit 1
}

set nfn [file join $datatopdir data autoselection.txt]
set ifh [open $infn r]
set ofh [open $nfn w]
puts $ofh "# autoselection"
puts $ofh "# [clock format [clock seconds] -gmt 1 -format {%Y-%m-%d %H:%M:%S}]"
while { [gets $ifh line] >= 0 } {
  if { [regexp {^#} $line] } {
    continue
  }
  regexp {^([^=]*)=(.*)$} $line all key value
  if { $key eq "version" } {
    set value 1
  }
  if { $key eq "tagdistance" } { set key histdistance }
  puts $ofh $key
  puts $ofh "..$value"
}
close $ifh
close $ofh
