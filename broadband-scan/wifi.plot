#!/usr/bin/gnuplot

set terminal png truecolor rounded size 1920,720 enhanced
set output "wifi.png"
set xtics 0.01
set mxtics 10
set grid xtics
set grid ytics
set object  1 rectangle from 2.401,0 to 2.423,100 fs solid fc rgb "#ffd0d0" behind
set object  6 rectangle from 2.426,0 to 2.448,100 fs solid fc rgb "#d0ffd0" behind
set object 11 rectangle from 2.451,0 to 2.473,100 fs solid fc rgb "#d0d0ff" behind
set format x "%.2fGHz"
set label  1 center at screen 0.15,0.95, char 1 "Channel 1"  font ",14"
set label  6 center at screen 0.38,0.95, char 1 "Channel 6"  font ",14"
set label 11 center at screen 0.61,0.95, char 1 "Channel 11" font ",14"
plot [2.4:2.5] "broadband-scan.txt" using ($2/1e9):3 with dots title "Signal"
