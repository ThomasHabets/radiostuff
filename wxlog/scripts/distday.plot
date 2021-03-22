#!/usr/bin/gnuplot
#
# Plot the distance seen by time of day.
#
set terminal pngcairo size 1280,480
set output "distday.png"
set title "Avg FT8 decode distance per per time slot"
set ylabel "km"
set xlabel "Hour (UTC)"

plot [0:24] [0:] \
  'distday20.data' using ($1/3600):2 w l title '20m', \
  'distday40.data' using ($1/3600):2 w l title '40m'
