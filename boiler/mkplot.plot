#!/usr/bin/gnuplot

set terminal pngcairo size 1280, 800
set output "boiler.png"
set timefmt "%s"
set ylabel "off / on"
set xdata time
set xtics rotate
set format x "%Y-%m-%d %H:%M"
plot [] [-1:2] 'onoff.data' using 1:2 w linespoints title "Boiler status"
