#!/bin/sh
# restart \
exec rlwrap tclsh "$0" ${1+"$@"}

## @file
# Simple CLI-like interface
#
# This is mainly for the purpose of testing if everything works as expected.
#
# @author Piotr S. Staszewski

set mode "9600,n,8,1";  #< Default device to open

## Write raw command.
# @param data The string to write
proc pwrite {data} {
	global port
	puts -nonewline $port [binary format ca* [string length $data] $data]
	flush $port
}

## Read raw data.
proc pread {} {
	global port
	read $port
}

## Print string on the LCD.
# @see cliBuffer
# @param str The string to print
proc fprint {str} {
  pwrite "p$str"
}

## Set LCD coordinates.
# @see lcd_goto
# @param x The horizontal coordinate
# @param y The vertical coordinate
proc fgoto {x y} {
  pwrite [binary format acc "g" $x $y]
}

## Dim the LCD.
# This will set the PWM duty cycle.
# @param x 8bit value
proc fdim {x} {
  pwrite [binary format ac "d" $x]
}

## Write a raw byte to the lcd.
# This will convert the params to appropriate C types.
# @param data The byte to send
# @param chars Character if true, command otherwise
# @param wait Wait additional 5 ms if true
proc fraw {data chars wait} {
  pwrite [
    binary format accc "r" $data [string is true $chars] [string is true $wait]
  ]
}

## Process the IR receiver input.
# @private
proc cmdr {} {
  global port
  binary scan [read $port 2] cc adr cmd
  set adr [expr {$adr & 0xff}]
  set cmd [expr {$cmd & 0xff}]
  puts "<<< (CMDR) adr: $adr\tcmd: $cmd"
}

## Main dispatcher for irpanel input.
# @private
proc dispatcher {} {
  global port
  if {![eof $port]} {
    set token [read $port 1]
    if {[llength [info commands "cmd$token"]] == 1} {
      fconfigure $port -blocking 1
      fileevent $port readable {}
      cmd$token
      fileevent $port readable dispatcher
      fconfigure $port -blocking 0
    } else {
      puts "<<< UNKNOWN: $token"
    }
  } else {
    puts "($device:EOF)"
  }
}

## Simple REPL input processor.
# @private
proc repl {} {
  gets stdin line
  catch $line result
  if {[string length $line] > 0} {
    puts "=== $result"
  }
  puts -nonewline "%%% "
  flush stdout
}

if {$::argc > 0} {
  set device [lindex $::argv 0]
} else {
  set device "/dev/ttyUSB0"
}

set port [open $device w+]
fconfigure $port -mode $mode -handshake none -blocking 0 -buffering full -buffersize 32 -translation binary
fileevent $port readable dispatcher
fileevent stdin readable repl
puts "($device:$mode:READY)"
puts -nonewline "%%% "
flush stdout

vwait forever
