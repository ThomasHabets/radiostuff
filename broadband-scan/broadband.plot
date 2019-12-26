#!/usr/bin/gnuplot

set terminal png truecolor rounded size 1920,720 enhanced
set output "broadband.png"
set xtics 500
set mxtics 5
set grid xtics
set grid mxtics
set grid ytics
plot [800:6000] "broadband-scan.txt" using ($2/1e6):3 with points title "Signal"
