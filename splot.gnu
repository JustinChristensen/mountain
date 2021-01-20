set term qt \
    size 1200, 800 \
    title "Memory Mountain" \
    font "Verdana"

set grid vertical
set hidden3d
set log y 2

set xyplane relative 0.005

set xlabel "stride" offset 0, -2
set xtics 1, 2 offset 0, -1

set ylabel "size (bytes)" offset 5, -1
set ytics 2 logscale
set format y '%3.0b%B'

set zlabel "throughput (MB/s)" offset -10, 0
set ztics 2500

MB_per_sec(stride, size, time) = (size / stride) / (time / 1000)
# splot "-" using 1:2:(MB_per_sec($1, $2, $3)) with lines
if (!exists("datafile")) datafile='run.txt'
splot datafile using 1:2:(MB_per_sec($1, $2, $3)) with lines palette

