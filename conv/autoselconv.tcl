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

set fn autosel.txt
puts "## Converting: $fn"
set ifh [open [file join $dir $fn] r]
set nfn [file join data autoselection.txt]
set ofh [open $nfn w]
puts $ofh "# BDJ4 autoselection"
puts $ofh "# [clock format [clock seconds] -gmt 1]"
puts $ofh "datafiletype"
puts $ofh "..simple"
while { [gets $ifh line] >= 0 } {
  if { [regexp {^#} $line] } {
    puts $ofh $line
    continue
  }
  regexp {^([^=]*)=(.*)$} $line all key value
  if { $key eq "version" } {
    set value 3
  }
  puts $ofh $key
  puts $ofh "..$value"
}
close $ifh
close $ofh
