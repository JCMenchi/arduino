#!/usr/bin/gnuplot -p


set datafile separator ',';

set xdata time;

set y1tics -2, 2
set y2tics 0, 3

plot "data.csv" using 0:2 with lines, "data.csv" using 0:3 with lines, "data.csv" using 0:4 with lines, "data.csv" using 0:11 axis x1y2 with lines

plot "data.csv" using 0:5 with lines, "data.csv" using 0:6 with lines, "data.csv" using 0:7 with lines, "data.csv" using 0:11 axis x1y2 with lines

plot "data.csv" using 0:8 with lines, "data.csv" using 0:9 with lines, "data.csv" using 0:10 with lines, "data.csv" using 0:11 axis x1y2 with lines