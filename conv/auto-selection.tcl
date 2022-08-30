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

# chances are very good that no changes were made to auto selection.
# simply copy over the template file (z-new.tcl).
# this code will be left here in case it is needed.

exit 0

set fn autosel.txt
set infn [file join $bdj3dir $fn]

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
  } else {
    set key [string toupper $key]
  }
  if { $key eq "TAGDISTANCE" } { set key HISTDISTANCE }
  if { $key eq "LOW" } { set key COUNTLOW }
  puts $ofh $key
  if { $key eq "HISTDISTANCE" || $key eq "version" || $key eq "BEGINCOUNT" } {
    puts $ofh "..$value"
  } else {
    set value [expr {int($value * 1000)}];
    puts $ofh "..$value"
  }
}
close $ifh
close $ofh
exit 0
