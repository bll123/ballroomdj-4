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

set infn [file join $bdj3dir status.tcl]
set nfn [file join $datatopdir data status.txt]

if { ! [file exists $infn] } {
  puts "   status: copying template"
  file copy -force templates/status.txt $nfn
  exit 0
}

source $infn
set fh [open $nfn w]
puts $fh "# status"
puts $fh "# [clock format [clock seconds] -gmt 1 -format {%Y-%m-%d %H:%M:%S}]"
puts $fh version
puts $fh "..1"
puts $fh count
set count [expr {[llength $Status]/2}]
puts $fh ..${count}
foreach {key data} $Status {
  puts $fh "KEY\n..$key"
  set nplayflag 0
  foreach {k v} $data {
    if { $k eq "oktoplay" } { set k playflag }
    set k [string toupper $k]
    if { $k eq "STATUS" && $v eq "New" && $count == 2 } {
      set nplayflag 1
    }
    if { $k eq "PLAYFLAG" && $nplayflag == 1 } {
      # want all statuses to be playable by default
      # where there are just 'new' and 'complete'.
      # the user can change them.
      set v 1
    }
    puts $fh $k
    puts $fh "..$v"
  }
}
close $fh

