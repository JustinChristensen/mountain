# set term qt \
#     size 1400, 800 \
#     title "Memory Mountain" \
#     font "Verdana"

set term svg \
    size 1800, 1200 dynamic \
    font "Verdana" \
    background "white"

set view 55, 69, 0.9
set grid vertical
set hidden3d
set log y 2

set xyplane relative 0.005

set key spacing 2 font ",12"
set title "throughput (MB/s) for reads of size / stride bytes" \
    font ",16" offset 0,-2

set xlabel "stride (uint64_t)" offset 0, -5 font ",12" noenhanced
set xtics 1, 2 offset 0, -1

set ylabel "size (bytes)" offset 0, -4 font ",12" noenhanced
set ytics 2 logscale offset 0, -1
set format y '%3.0b%B'

set zlabel "throughput (MB/s)" offset -12, 0 font ",12" noenhanced
set ztics 2500

MB_per_sec(stride, size, time) = (size / stride) / (time / 1000)
cache_by_color(size) =  size <= (1 << 15) ? 0xf7d367 : \
                        size <= (1 << 18) ? 0xff0000 : \
                        size <= (1 << 23) ? 0x0000ff : \
                                           0x0

set pm3d nolighting border lw 2 lc rgb "gray" solid
unset colorbox

if (!exists("datafile")) datafile='run.txt'
splot datafile \
    using 1:2:(MB_per_sec($1, $2, $3)):(cache_by_color($2)) \
    notitle \
    with pm3d lc rgb variable, \
    keyentry with lines lc rgb 0xf7d367 lw 5 title "L1 (32K)  ", \
    keyentry with lines lc rgb 0xff0000 lw 5 title "L2 (256K)  ", \
    keyentry with lines lc rgb 0x0000ff lw 5 title "L3 (8M)  ", \
    keyentry with lines lc rgb 0x0 lw 4 title "Main Memory  "
