#!/usr/bin/gnuplot
#
# Plot the records seen in a day.
#
set terminal pngcairo size 1280,480
set output "day.png"

plot [0:24] [0:] \
  'day20.data' using ($1/3600):2 w l title '20m', \
  'day40.data' using ($1/3600):2 w l title '40m'
